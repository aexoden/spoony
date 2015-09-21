/*
 * Copyright (c) 2015 Jason Lynch <jason@calindora.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iomanip>
#include <map>

#include "engine.hh"
#include "version.hh"

std::vector<int> FF4_RNG_DATA{
	0x07, 0xB6, 0xF0, 0x1F, 0x55, 0x5B, 0x37, 0xE3, 0xAE, 0x4F, 0xB2, 0x5E, 0x99, 0xF6, 0x77, 0xCB,
	0x60, 0x8F, 0x43, 0x3E, 0xA7, 0x4C, 0x2D, 0x88, 0xC7, 0x68, 0xD7, 0xD1, 0xC2, 0xF2, 0xC1, 0xDD,
	0xAA, 0x93, 0x16, 0xF7, 0x26, 0x04, 0x36, 0xA1, 0x46, 0x4E, 0x56, 0xBE, 0x6C, 0x6E, 0x80, 0xD5,
	0xB5, 0x8E, 0xA4, 0x9E, 0xE7, 0xCA, 0xCE, 0x21, 0xFF, 0x0F, 0xD4, 0x8C, 0xE6, 0xD3, 0x98, 0x47,
	0xF4, 0x0D, 0x15, 0xED, 0xC4, 0xE4, 0x35, 0x78, 0xBA, 0xDA, 0x27, 0x61, 0xAB, 0xB9, 0xC3, 0x7D,
	0x85, 0xFC, 0x95, 0x6B, 0x30, 0xAD, 0x86, 0x00, 0x8D, 0xCD, 0x7E, 0x9F, 0xE5, 0xEF, 0xDB, 0x59,
	0xEB, 0x05, 0x14, 0xC9, 0x24, 0x2C, 0xA0, 0x3C, 0x44, 0x69, 0x40, 0x71, 0x64, 0x3A, 0x74, 0x7C,
	0x84, 0x13, 0x94, 0x9C, 0x96, 0xAC, 0xB4, 0xBC, 0x03, 0xDE, 0x54, 0xDC, 0xC5, 0xD8, 0x0C, 0xB7,
	0x25, 0x0B, 0x01, 0x1C, 0x23, 0x2B, 0x33, 0x3B, 0x97, 0x1B, 0x62, 0x2F, 0xB0, 0xE0, 0x73, 0xCC,
	0x02, 0x4A, 0xFE, 0x9B, 0xA3, 0x6D, 0x19, 0x38, 0x75, 0xBD, 0x66, 0x87, 0x3F, 0xAF, 0xF3, 0xFB,
	0x83, 0x0A, 0x12, 0x1A, 0x22, 0x53, 0x90, 0xCF, 0x7A, 0x8B, 0x52, 0x5A, 0x49, 0x6A, 0x72, 0x28,
	0x58, 0x8A, 0xBF, 0x0E, 0x06, 0xA2, 0xFD, 0xFA, 0x41, 0x65, 0xD2, 0x4D, 0xE2, 0x5C, 0x1D, 0x45,
	0x1E, 0x09, 0x11, 0xB3, 0x5F, 0x29, 0x79, 0x39, 0x2E, 0x2A, 0x51, 0xD9, 0x5D, 0xA6, 0xEA, 0x31,
	0x81, 0x89, 0x10, 0x67, 0xF5, 0xA9, 0x42, 0x82, 0x70, 0x9D, 0x92, 0x57, 0xE1, 0x3D, 0xF1, 0xF9,
	0xEE, 0x08, 0x91, 0x18, 0x20, 0xB1, 0xA5, 0xBB, 0xC6, 0x48, 0x50, 0x9A, 0xD6, 0x7F, 0x7B, 0xE9,
	0x76, 0xDF, 0x32, 0x6F, 0x34, 0xA8, 0xD0, 0xB8, 0x63, 0xC8, 0xC0, 0xEC, 0x4B, 0xE8, 0x17, 0xF8
};

Engine::Engine(const Parameters & parameters, const std::vector<std::shared_ptr<const Instruction>> & instructions, const Encounters & encounters) :
	_parameters{parameters},
	_instructions{instructions},
	_encounters{encounters}
{
	_reset(_parameters.seed);
}

void Engine::run()
{
	while (_instruction_index < _instructions.size())
	{
		_cycle();
	}
}

Glib::ustring format_label(const Glib::ustring & label)
{
	return Glib::ustring::format(std::left, std::setw(18), label);
}

Glib::ustring format_time(double frames)
{
	return Glib::ustring::format(std::fixed, std::setprecision(3), frames / 60.0988);
}

Glib::ustring Engine::format_output(const Engine & base_engine)
{
	Glib::ustring output;

	int total_optional_steps = 0;
	int total_extra_steps = 0;

	output.append(Glib::ustring::compose("ROUTE\t%1\n", _title));
	output.append(Glib::ustring::compose("VERSION\t%1\n", _version));
	output.append(Glib::ustring::compose("SPOONY\t%1\n", SPOONY_VERSION));
	output.append(Glib::ustring::compose("SEED\t%1\n", _parameters.seed));
	output.append(Glib::ustring::compose("MAXSTEP\t%1\n", _parameters.maximum_extra_steps));
	output.append(Glib::ustring::compose("FRAMES\t%1\n", _frames));
	output.append("\n");

	for (auto & entry : _log)
	{
		Glib::ustring indent = "";

		for (int i = 0; i < entry.indent; i++)
		{
			indent.append("  ");
		}

		output.append(Glib::ustring::compose("%1%2\n", indent, entry.instruction->text));

		int optional_steps = std::min(entry.instruction->optional_steps, entry.steps - entry.instruction->required_steps);
		int extra_steps = entry.steps - entry.instruction->required_steps - optional_steps;

		if (extra_steps % 2 == 1 && optional_steps > 0)
		{
			extra_steps++;
			optional_steps--;
		}

		total_optional_steps += optional_steps;
		total_extra_steps += extra_steps;

		if (entry.instruction->optional_steps > 0)
		{
			output.append(Glib::ustring::compose("%1  Optional Steps: %2\n", indent, optional_steps));
		}

		if (extra_steps > 0)
		{
			output.append(Glib::ustring::compose("%1  Extra Steps: %2\n", indent, extra_steps));
		}

		for (auto & pair : entry.encounters)
		{
			output.append(Glib::ustring::compose("%1  Step %2: %3 (%4s)\n", indent, Glib::ustring::format(std::setw(3), pair.first), pair.second->get_description(), format_time(pair.second->get_average_duration())));
		}
	}

	output.append("\n");

	output.append(Glib::ustring::compose("%1 %2s\n", format_label("Encounter Time:"), format_time(_encounter_frames)));
	output.append(Glib::ustring::compose("%1 %2s\n", format_label("Other Time:"), format_time(_frames - _encounter_frames)));
	output.append(Glib::ustring::compose("%1 %2s\n", format_label("Total Time:"), format_time(_frames)));
	output.append("\n");
	output.append(Glib::ustring::compose("%1 %2s\n", format_label("Base Total Time:"), format_time(base_engine._frames)));
	output.append(Glib::ustring::compose("%1 %2s\n", format_label("Time Saved:"), format_time(base_engine._frames - _frames)));
	output.append("\n");
	output.append(Glib::ustring::compose("%1 %2\n", format_label("Optional Steps:"), total_optional_steps));
	output.append(Glib::ustring::compose("%1 %2\n", format_label("Extra Steps:"), total_extra_steps));
	output.append(Glib::ustring::compose("%1 %2\n", format_label("Encounters:"), _encounter_count));
	output.append("\n");
	output.append(Glib::ustring::compose("%1 %2\n", format_label("Base Encounters:"), base_engine._encounter_count));
	output.append(Glib::ustring::compose("%1 %2\n", format_label("Encounters Saved:"), base_engine._encounter_count - _encounter_count));

	return output;
}

/*
		output += '\n'
		output += '{:18} {:.3f}s\n'.format('Encounter Time:', frames_to_seconds(self._encounter_frames))
		output += '{:18} {:.3f}s\n'.format('Other Time:', frames_to_seconds(self._frames - self._encounter_frames))
		output += '{:18} {:.3f}s\n'.format('Total Time:', frames_to_seconds(self._frames))
		output += '\n'
		output += '{:18} {:.3f}s\n'.format('Base Total Time:', frames_to_seconds(base_engine._frames))
		output += '{:18} {:.3f}s\n'.format('Time Saved:', frames_to_seconds(base_engine._frames - self._frames))
		output += '\n'
		output += '{:18} {}\n'.format('Optional Steps:', total_optional_steps)
		output += '{:18} {}\n'.format('Extra Steps:', total_extra_steps)
		output += '{:18} {}\n'.format('Encounters:', self._encounter_count)
		output += '\n'
		output += '{:18} {}\n'.format('Base Encounters:', base_engine._encounter_count)
		output += '{:18} {}\n'.format('Encounters Saved:', base_engine._encounter_count - self._encounter_count)
*/

void Engine::_cycle()
{
	if (_instruction_index == _instructions.size())
	{
		return;
	}

	auto instruction = _instructions[_instruction_index];

	switch (instruction->type)
	{
		case InstructionType::CHOICE:
			break;
		case InstructionType::NOOP:
			break;
		case InstructionType::NOTE:
			break;
		case InstructionType::OPTION:
		case InstructionType::END:
			break;
		case InstructionType::PATH:
			_transition(instruction);
			_encounter_rate = instruction->encounter_rate;
			_encounter_group = instruction->encounter_group;
			_step(instruction->tiles, instruction->required_steps);

			if (instruction->optional_steps > 0 || (instruction->take_extra_steps && (instruction->can_single_step || instruction->can_double_step)))
			{
				int steps = _parameters.randomizer->get_int(0, _parameters.maximum_extra_steps);
				int optional_steps = std::min(instruction->optional_steps, steps);
				int extra_steps = steps - optional_steps;
				int tiles = 0;

				if (extra_steps % 2 == 1 && optional_steps > 0)
				{
					extra_steps++;
					optional_steps--;
				}

				if (extra_steps % 2 == 1 && !instruction->can_single_step)
				{
					extra_steps++;
				}

				if (instruction->can_double_step)
				{
					tiles = extra_steps;
				}
				else
				{
					tiles = extra_steps * 2;
				}

				if (tiles % 2 == 1)
				{
					tiles++;
				}

				_step(tiles, optional_steps + extra_steps);
			}

			break;
		case InstructionType::ROUTE:
			_title = instruction->text;
			break;
		case InstructionType::SEARCH:
			break;
		case InstructionType::VERSION:
			_version = instruction->number;
			break;
		case InstructionType::WAIT:
			break;
	}

	_instruction_index++;
}

std::shared_ptr<const Encounter> Engine::_get_encounter()
{
	if ((FF4_RNG_DATA[_step_index] + _step_seed) % 256 < _encounter_rate)
	{
		int value = (FF4_RNG_DATA[_encounter_index] + _encounter_seed) % 256;
		int i;

		if (value < 43)
		{
			i = 0;
		}
		else if (value < 86)
		{
			i = 1;
		}
		else if (value < 129)
		{
			i = 2;
		}
		else if (value < 172)
		{
			i = 3;
		}
		else if (value < 204)
		{
			i = 4;
		}
		else if (value < 236)
		{
			i = 5;
		}
		else if (value < 252)
		{
			i = 6;
		}
		else
		{
			i = 7;
		}

		return _encounters.get_encounter(_encounter_group, i);
	}

	return nullptr;
}

void Engine::_reset(int seed)
{
	_step_seed = seed;
	_encounter_seed = (seed * 2) % 256;

	_step_index = 0;
	_encounter_index = 0;

	_encounter_rate = 0;
	_encounter_group = 0;
}

void Engine::_step(int tiles, int steps)
{
	_frames += tiles * 16;

	for (int i = 0; i < steps; i++)
	{
		_step_index = (_step_index + 1) % 256;
		_log.back().steps++;

		if (_step_index == 0)
		{
			_step_seed = (_step_seed + 17) % 256;
		}

		auto encounter = _get_encounter();

		if (encounter && _encounter_search && encounter->get_id() == _encounter_search->get_id())
		{
			_encounter_search = nullptr;
		}

		if (encounter)
		{
			_log.back().encounters[_log.back().steps] = encounter;
			_frames += encounter->get_average_duration();
			_encounter_frames += encounter->get_average_duration();
			_encounter_count++;

			_encounter_index = (_encounter_index + 1) % 256;

			if (_encounter_index == 0)
			{
				_encounter_seed = (_encounter_seed + 17) % 256;
			}
		}
	}
}

void Engine::_transition(const std::shared_ptr<const Instruction> & instruction)
{
	_frames += instruction->transition_count * 82;
	_log.push_back(LogEntry{instruction, _indent});
}

LogEntry::LogEntry(const std::shared_ptr<const Instruction> & instruction, int indent) :
	instruction(instruction),
	indent(indent)
{ }
