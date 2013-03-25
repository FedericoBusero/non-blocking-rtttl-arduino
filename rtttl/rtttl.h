/** TODO:CH 
 * Introduce skip logic which handles any whitespace insertion; cases skip("o=") ["o=", "o = "] or skip(",") [",", " , "]. 
 * Introduce note detection which is case-insensitive
 * Handle variations in sequence by permitting valid instructions in any sequence
 * */

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
# include "Arduino.h"
#else
# include "WProgram.h"
# include "pins_arduino.h"
#endif

#include <avr/pgmspace.h>
#include <notes.h>

#define isdigit(n) (n >= '0' && n <= '9')

#ifndef NULL
  #define NULL 0
#endif

const prog_uint16_t notes[] PROGMEM =
{	0, //
	NOTE_C4, //
	NOTE_CS4, //
	NOTE_D4, //
	NOTE_DS4, //
	NOTE_E4, //
	NOTE_F4, //
	NOTE_FS4, //
	NOTE_G4, //
	NOTE_GS4, //
	NOTE_A4, //
	NOTE_AS4, //
	NOTE_B4, //

	NOTE_C5, //
	NOTE_CS5, //
	NOTE_D5, //
	NOTE_DS5, //
	NOTE_E5, //
	NOTE_F5, //
	NOTE_FS5, //
	NOTE_G5, //
	NOTE_GS5, //
	NOTE_A5, //
	NOTE_AS5, //
	NOTE_B5, //

	NOTE_C6, //
	NOTE_CS6, //
	NOTE_D6, //
	NOTE_DS6, //
	NOTE_E6, //
	NOTE_F6, //
	NOTE_FS6, //
	NOTE_G6, //
	NOTE_GS6, //
	NOTE_A6, //
	NOTE_AS6, //
	NOTE_B6, //

	NOTE_C7, //
	NOTE_CS7, //
	NOTE_D7, //
	NOTE_DS7, //
	NOTE_E7, //
	NOTE_F7, //
	NOTE_FS7, //
	NOTE_G7, //
	NOTE_GS7, //
	NOTE_A7, //
	NOTE_AS7, //
	NOTE_B7, //

	2*NOTE_C7, //
	2*NOTE_CS7, //
	2*NOTE_D7, //
	2*NOTE_DS7, //
	2*NOTE_E7, //
	2*NOTE_F7, //
	2*NOTE_FS7, //
	2*NOTE_G7, //
	2*NOTE_GS7, //
	2*NOTE_A7, //
	2*NOTE_AS7, //
	2*NOTE_B7, //
	0
};

class Song {

	protected:

		uint8_t tonePin; //the pin to which the speaker is attached
		
		int nextPos; //the integer position of the next byte to read from the song stream 

		unsigned long periodStart; //when the last note or rest was started
		unsigned long periodLength; //the length of the last note or rest

		int bpm; //the beats per minute (speed) of the song
		unsigned long wholenote; //duration of a whole note in milliseconds 
		byte defaultNoteDenominator; //default fraction of a wholenote (if not specified against a note)
		
		byte defaultOctave; //the default octave (if not specified against a note)
		uint8_t transposeOctaves; //always transpose by this number of octaves before playing notes
		
#ifdef _Tone_h
		Tone m_tone;
#endif
	
    public:
    
		Song(uint8_t tonePin){
			this->tonePin = tonePin;
			this->nextPos = 0;
			this->defaultNoteDenominator = 4;
			this->defaultOctave = 6;
			this->bpm = 63;
		}
						
		bool initSong()
		{
			
			int num; //a temporary integer working space for various calculations 

			//Serial.println("initSong() started");			

	#ifdef _Tone_h
			this->m_tone.begin(tonePin);
	#endif
			
			//prepare variables which will keep track of place in the song
			periodStart = -1;
			nextPos = 0;
			
			// format: d=N,o=N,b=NNN:
			// find the start (skip name, etc)

			while (peek_byte() != ':'){
				pop_byte(); //(ignore name characters)
			}
						
			pop_byte(); // skip the ':' character

			//Serial.println("END NAME");

			// get default duration
			if (peek_byte() == 'd')
			{
				pop_byte();
				pop_byte(); // skip "d="
				num = 0;
				while (isdigit(peek_byte()))
				{
					num = (num * 10) + (pop_byte() - '0');
				}
				if (num > 0)
					defaultNoteDenominator = num;
				pop_byte(); // skip comma
			}

			//Serial.println("END DURATION");

			// get default octave
			if (peek_byte() == 'o')
			{
				pop_byte(); 
				pop_byte(); // skip "o="
				num = pop_byte() - '0';
				if (num >= 3 && num <= 7)
					defaultOctave = num;
				pop_byte(); // skip comma
			}

			//Serial.println("END DEF OCT");

			// get BPM
			if (peek_byte() == 'b')
			{
				pop_byte();
				pop_byte(); // skip "b="
				num = 0;
				while (isdigit(peek_byte()))
				{
					num = (num * 10) + (pop_byte() - '0');
				}
				bpm = num;
				pop_byte(); // skip colon
			}

			//Serial.println("END BPM");

			// BPM usually expresses the number of quarter notes per minute
			wholenote = (60 * 1000L / bpm) * 4; // this is the time for whole note (in milliseconds)
					
			return true;

		}
		
		bool tick()
		{
			//see if a new transition needs to be processed
			if(periodStart == -1 || (millis() - periodStart) > periodLength){
				//remember when it started
				periodStart = millis();
				//process next transition, and check if it is the last
				if(!nextTransition()){
					initSong();
					return false;
				}
			}
			return true;
		}
		
	private:

#ifdef _Tone_h
		void _tone(uint16_t freq)
		{
			this->m_tone.play(freq);
		}

		void _noTone()
		{
			this->m_tone.stop();
		}
#else
		void _tone(uint16_t freq)
		{
			tone(this->tonePin, freq);
		}

		void _noTone()
		{
			noTone(this->tonePin);
		}
#endif
		
		bool nextTransition()
		{
			
			//Serial.println("nextTransition() started");

			byte note;
			byte scale;
			
			// read a single byte - test for end of song
			if(!peek_byte()){
				_noTone();
				return false;
			}

			//Serial.println("SONG CONTINUING");
			
			// first, get note duration, if available
			int denominator = 0;
			while (isdigit(peek_byte()))
			{
				denominator = (denominator * 10) + (pop_byte() - '0');
			}

			//Serial.println("END DURATION");

			if (denominator)
				periodLength = wholenote / denominator;
			else
				periodLength = wholenote / defaultNoteDenominator; // we will need to check if we are a dotted note after

			// now get the note
			note = 0;

			switch (pop_byte())
			{
			case 'c':
				note = 1;
				break;
			case 'd':
				note = 3;
				break;
			case 'e':
				note = 5;
				break;
			case 'f':
				note = 6;
				break;
			case 'g':
				note = 8;
				break;
			case 'a':
				note = 10;
				break;
			case 'b':
				note = 12;
				break;
			case 'p':
			default:
				note = 0;
			}

			//Serial.println("END NOTE");

			// now, get optional '#' sharp
			if (peek_byte() == '#')
			{
				pop_byte();
				note++;
			}

			//Serial.println("END #");

			// now, get optional '.' dotted note
			if (peek_byte() == '.')
			{
				pop_byte();
				periodLength += periodLength / 2;
			}

			//Serial.println("END .");

			// now, get scale
			if (isdigit(peek_byte()))
			{
				scale = pop_byte() - '0';
			}
			else
			{
				scale = defaultOctave;
			}

			scale += transposeOctaves;

			//Serial.println("END SCALE");

			if (peek_byte() == ',')
				pop_byte(); // skip comma for next note (or we may be at the end)

			// now play the note

			if (note)
			{
				uint16_t note_word = pgm_read_word(&notes[(scale - 4) * 12 + note]);
				_tone(note_word);
				//Serial.print("Playing note: ");
				//Serial.println(note_word);
			}
			else
			{
				//Serial.println("Playing rest");
				_noTone();
			}

			return true;

		}
						
		char peek_byte(){
			return get_byte(nextPos);
			/*
			char read = get_byte(nextPos);
			Serial.print("Peeked: '");
			Serial.write(read);
			Serial.println("'");
			return read;
			*/
		}
		
		char pop_byte(){
			get_byte(nextPos++);
			/*
			char read = get_byte(nextPos++);
			//Serial.print("Popped: '");
			//Serial.write(read);
			//Serial.println("'");
			return read;
			*/
		}
				
		virtual char get_byte(int pos){ return 0; };
		
};

class ConstSong: public Song{
	
	protected:
		const char* songStart; //the string containing the song (in RTTTL format)

	public:

		ConstSong(uint8_t tonePin)
			:Song(tonePin) //call superclass constructor
		{
			//do nothing
		}
		
		void setSong(const char* song){
			this->songStart = song;
			Song::initSong();
		}

};


class ProgmemSong: public ConstSong{

	public:
		ProgmemSong(uint8_t tonePin)
			:ConstSong(tonePin)
		{
			//do nothing
		}

	private:
		char get_byte(int pos)
		{
			return pgm_read_byte(songStart + pos);
		}

};

class RamSong: public ConstSong{

	public:
		RamSong(uint8_t tonePin)
			:ConstSong(tonePin)
		{
			//do nothing
		}
		

	private:
		char get_byte(int pos)
		{
			return *(songStart + pos);
		}

};
