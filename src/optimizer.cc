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
#include <iostream>
#include <random>

#include "engine.hh"
#include "optimizer.hh"
#include "options.hh"
#include "randomizer.hh"
#include "route_output.hh"

void optimize_bb(int start_index, double & best_frames, int & best_score, const Options & options, const std::shared_ptr<Randomizer> & randomizer, Engine & engine, const Engine & base_engine, const Glib::RefPtr<Gio::File> & output_file)
{
	randomizer->reset();

	engine.reset();
	engine.run();

	double search_best_frames = engine.get_frames();
	int search_best_score = randomizer->get_score();

	std::vector<int> best_data;

	int index = start_index;

	int iterations = 0;

	while (true)
	{
		if (iterations % 143 == 0)
		{
			std::cout << "\rSeed: " << std::right << std::setw(4) << options.seed;
			std::cout << "   Algorithm: " << std::left << std::setw(15) << "Branch and Bound";
			std::cout << "   Index: " << std::right << std::setw(2) << index;
			std::cout << "   Iterations: " << std::setw(20) << iterations;
			std::cout << "   Variables: (" << std::setw(2) << randomizer->get_minimum_variables() << ", " << randomizer->get_maximum_variables() << ")";
			std::cout << "   Best: " << std::setw(10) << Engine::frames_to_seconds(best_frames);
			std::cout << "   Search Best: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
			std::cout << std::flush;
		}

		randomizer->reset();
		randomizer->set_implicit_index(index);

		engine.reset();
		engine.run();

		iterations++;

		if ((engine.get_frames() < best_frames || (engine.get_frames() == best_frames && randomizer->get_score() > best_score)) && RouteOutput::write_route(output_file, randomizer, engine, base_engine, false))
		{
			best_frames = engine.get_frames();
			best_score = randomizer->get_score();
		}

		if (engine.get_frames() < search_best_frames || (engine.get_frames() == search_best_frames && randomizer->get_score() > search_best_score))
		{
			best_data = randomizer->data;
			search_best_frames = engine.get_frames();
			search_best_score = randomizer->get_score();
		}

		if (randomizer->data[index] == options.maximum_steps || engine.get_minimum_frames() > best_frames)
		{
			randomizer->data[index] = 0;
			index--;

			if (index >= 0)
			{
				randomizer->data[index]++;
			}
		}
		else
		{
			if (index + 1 < randomizer->get_index())
			{
				index++;
			}
			else
			{
				randomizer->data[index]++;
			}
		}

		if (index == start_index - 1)
		{
			randomizer->data = best_data;
			break;
		}
	}

	randomizer->reset();

	engine.reset();
	engine.run();

	std::cout << std::endl;
}

void optimize_ils(int start_index, double & best_frames, int & best_score, const Options & options, const std::shared_ptr<Randomizer> & randomizer, Engine & engine, const Engine & base_engine, const Glib::RefPtr<Gio::File> & output_file)
{
	std::random_device rd;
	std::default_random_engine random_engine{rd()};

	optimize_local(start_index, best_frames, best_score, options, randomizer, engine, base_engine, output_file, false);

	double search_best_frames = engine.get_frames();

	for (int i = 0; i < options.maximum_iterations; i++)
	{
		std::cout << "\rSeed: " << std::right << std::setw(4) << options.seed;
		std::cout << "   Algorithm: " << std::left << std::setw(15) << "ILS";
		std::cout << "   Index: " << std::right << std::setw(2) << i;
		std::cout << "   Variables: (" << std::setw(2) << randomizer->get_minimum_variables() << ", " << randomizer->get_maximum_variables() << ")";
		std::cout << "   Best: " << std::setw(10) << Engine::frames_to_seconds(best_frames);
		std::cout << "   Current: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
		std::cout << "   Search Best: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
		std::cout << "   Iteration: " << std::setw(10) << (i + 1);
		std::cout << std::flush;

		std::vector<int> current_data{randomizer->data};

		std::uniform_int_distribution<int> index_dist{start_index, static_cast<int>(randomizer->data.size()) - 1};

		int total = std::uniform_int_distribution<int>{-options.perturbation_wobble, options.perturbation_wobble}(random_engine);

		for (int j = 0; j < options.perturbation_strength; j++)
		{
			int index = index_dist(random_engine);
			total += randomizer->data[index];

			int value = std::uniform_int_distribution<int>{0, std::max(0, total * 2)}(random_engine);
			value = std::max(0, std::min(value, options.maximum_steps));

			if (j == options.perturbation_strength - 1)
			{
				value = std::max(0, std::min(total, options.maximum_steps));
			}

			randomizer->data[index] = value;
			total -= value;
		}

		optimize_local(start_index, best_frames, best_score, options, randomizer, engine, base_engine, output_file, false);

		if (engine.get_frames() < search_best_frames)
		{
			search_best_frames = engine.get_frames();
		}
		else
		{
			randomizer->data = current_data;
		}
	}

	randomizer->reset();

	engine.reset();
	engine.run();

	std::cout << std::endl;
}

void optimize_local(int start_index, double & best_frames, int & best_score, const Options & options, const std::shared_ptr<Randomizer> & randomizer, Engine & engine, const Engine & base_engine, const Glib::RefPtr<Gio::File> & output_file, bool final_newline)
{
	randomizer->reset();

	engine.reset();
	engine.run();

	double search_best_frames = engine.get_frames();

	while (true)
	{
		int best_index = -1;
		int best_value = 0;

		for (decltype(randomizer->data)::size_type i = start_index; i < randomizer->data.size(); i++)
		{
			std::cout << "\rSeed: " << std::right << std::setw(4) << options.seed;
			std::cout << "   Algorithm: " << std::left << std::setw(15) << "Local";
			std::cout << "   Index: " << std::right << std::setw(2) << i;
			std::cout << "   Variables: (" << std::setw(2) << randomizer->get_minimum_variables() << ", " << randomizer->get_maximum_variables() << ")";
			std::cout << "   Best: " << std::setw(10) << Engine::frames_to_seconds(best_frames);
			std::cout << "   Current: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
			std::cout << std::flush;

			int original_value = randomizer->data[i];

			for (int value = 0; value <= options.maximum_steps; value++)
			{
				randomizer->reset();
				randomizer->data[i] = value;

				engine.reset();
				engine.run();

				if ((engine.get_frames() < best_frames) && RouteOutput::write_route(output_file, randomizer, engine, base_engine, false))
				{
					best_frames = engine.get_frames();
					best_score = randomizer->get_score();
				}

				if (engine.get_frames() < search_best_frames)
				{
					search_best_frames = engine.get_frames();
					best_index = i;
					best_value = value;
				}
			}

			randomizer->data[i] = original_value;
		}

		if (best_index < 0)
		{
			break;
		}

		randomizer->data[best_index] = best_value;
	}

	randomizer->reset();

	engine.reset();
	engine.run();

	if (final_newline)
	{
		std::cout << std::endl;
	}
}

void optimize_local_pair(int start_index, double & best_frames, int & best_score, const Options & options, const std::shared_ptr<Randomizer> & randomizer, Engine & engine, const Engine & base_engine, const Glib::RefPtr<Gio::File> & output_file)
{
	randomizer->reset();

	engine.reset();
	engine.run();

	double search_best_frames = engine.get_frames();
	double previous_search_best_frames = engine.get_frames();
	int search_best_score = randomizer->get_score();

	while (true)
	{
		int best_i = -1;
		int best_j = -1;

		int best_i_value = 0;
		int best_j_value = 0;

		for (decltype(randomizer->data)::size_type i = start_index; i < randomizer->data.size(); i++)
		{
			int original_i_value = randomizer->data[i];

			auto maximum = randomizer->data.size();

			if (options.maximum_comparisons > 0)
			{
				maximum = std::min(maximum, i + 1 + options.maximum_comparisons);
			}

			for (decltype(randomizer->data)::size_type j = i + 1; j < maximum; j++)
			{
				std::cout << "\rSeed: " << std::right << std::setw(4) << options.seed;
				std::cout << "   Algorithm: " << std::left << std::setw(15) << "Pairwise Local";
				std::cout << "   Current Indexes: (" << std::right << std::setw(3) << i << ", " << std::setw(3) << j << ")";
				std::cout << "   Variables: (" << std::setw(2) << randomizer->get_minimum_variables() << ", " << randomizer->get_maximum_variables() << ")";
				std::cout << "   Best: " << std::setw(10) << Engine::frames_to_seconds(best_frames);
				std::cout << "   Previous: " << std::setw(10) << Engine::frames_to_seconds(previous_search_best_frames);
				std::cout << "   Current: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
				std::cout << "   " << search_best_score;
				std::cout << std::flush;

				int original_j_value = randomizer->data[j];

				for (int i_value = 0; i_value <= options.maximum_steps; i_value++)
				{
					int j_minimum = 0;
					int j_maximum = options.maximum_steps;

					if (options.pairwise_shift)
					{
						j_minimum = original_i_value + original_j_value - i_value;
						j_maximum = j_minimum;
					}

					for (int j_value = j_minimum; j_value <= j_maximum; j_value++)
					{
						if (j_value >= 0)
						{
							randomizer->reset();
							randomizer->data[i] = i_value;
							randomizer->data[j] = j_value;

							engine.reset();
							engine.run();

							if ((engine.get_frames() < best_frames || (engine.get_frames() == best_frames && randomizer->get_score() > best_score)) && RouteOutput::write_route(output_file, randomizer, engine, base_engine, false))
							{
								best_frames = engine.get_frames();
								best_score = randomizer->get_score();
							}

							if (engine.get_frames() < search_best_frames || (engine.get_frames() == search_best_frames && randomizer->get_score() > search_best_score))
							{
								search_best_frames = engine.get_frames();
								search_best_score = randomizer->get_score();
								best_i = i;
								best_j = j;
								best_i_value = i_value;
								best_j_value = j_value;
							}
						}
					}
				}

				randomizer->data[j] = original_j_value;
			}

			randomizer->data[i] = original_i_value;
		}

		if (best_i < 0)
		{
			break;
		}

		std::cout << std::endl << "Updating (" << best_i << ", " << best_j << ") from (" << randomizer->data[best_i] << ", " << randomizer->data[best_j] << ") to (" << best_i_value << ", " << best_j_value << ") (" << Engine::frames_to_seconds(previous_search_best_frames) << "s -> " << Engine::frames_to_seconds(search_best_frames) << "s)" << std::endl;
		previous_search_best_frames = search_best_frames;

		randomizer->data[best_i] = best_i_value;
		randomizer->data[best_j] = best_j_value;
	}

	randomizer->reset();

	engine.reset();
	engine.run();

	std::cout << std::endl;
}

void optimize_sequential(int start_index, double & best_frames, int & best_score, const Options & options, const std::shared_ptr<Randomizer> & randomizer, Engine & engine, const Engine & base_engine, const Glib::RefPtr<Gio::File> & output_file)
{
	randomizer->reset();

	engine.reset();
	engine.run();

	double search_best_frames = engine.get_frames();
	int search_best_score = randomizer->get_score();

	for (decltype(randomizer->data)::size_type i = start_index; i < randomizer->data.size(); i++)
	{
		std::cout << "\rSeed: " << std::right << std::setw(4) << options.seed;
		std::cout << "   Algorithm: " << std::left << std::setw(15) << "Sequential";
		std::cout << "   Index: " << std::right << std::setw(2) << i;
		std::cout << "   Variables: (" << std::setw(2) << randomizer->get_minimum_variables() << ", " << randomizer->get_maximum_variables() << ")";
		std::cout << "   Best: " << std::setw(10) << Engine::frames_to_seconds(best_frames);
		std::cout << "   Current: " << std::setw(10) << Engine::frames_to_seconds(search_best_frames);
		std::cout << std::flush;

		int best_value = randomizer->data[i];

		for (int value = 0; value <= options.maximum_steps; value++)
		{
			randomizer->reset();
			randomizer->data[i] = value;

			engine.reset();
			engine.run();

			if ((engine.get_frames() < best_frames || (engine.get_frames() == best_frames && randomizer->get_score() > best_score)) && RouteOutput::write_route(output_file, randomizer, engine, base_engine, false))
			{
				best_frames = engine.get_frames();
				best_score = randomizer->get_score();
			}

			if (engine.get_frames() < search_best_frames || (engine.get_frames() == search_best_frames && randomizer->get_score() > search_best_score))
			{
				search_best_frames = engine.get_frames();
				search_best_score = randomizer->get_score();
				best_value = value;
			}
		}

		randomizer->data[i] = best_value;
	}

	randomizer->reset();

	engine.reset();
	engine.run();

	std::cout << std::endl;
}
