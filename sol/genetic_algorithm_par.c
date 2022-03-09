#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "genetic_algorithm_par.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *P, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4)
	{
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count p\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL)
	{
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2)
	{
		fclose(fp);
		return 0;
	}

	if (*object_count % 10)
	{
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *)calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i)
	{
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2)
		{
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int)strtol(argv[2], NULL, 10);

	if (*generations_count == 0)
	{
		free(tmp_objects);

		return 0;
	}

	// se citeste numarul de thread-uri dat ca argument la apelul functiei
	*P = (int)strtol(argv[3], NULL, 10);

	if (P == 0)
	{
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i)
	{
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i)
	{
		for (int j = 0; j < generation[i].chromosome_length; ++j)
		{
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

// adaugare start si end pentru intervalul corespunzator thread-ului curent
void compute_fitness_function(const sack_object *objects, individual *generation, int sack_capacity, int start, int end)
{
	int weight;
	int profit;

	// se calculeaza doar pe intervalul thread-ului curent
	for (int i = start; i < end; ++i)
	{
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j)
		{
			if (generation[i].chromosomes[j])
			{
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0)
	{
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step)
		{
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
	else
	{
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step)
		{
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step)
	{
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i)
	{
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

// functie care merge-uieste doua intervale gata sortate dintr-un vector 
// intervalul final va si sortat
void merge(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;
	int i;

	for (i = start; i < end; i++) {
		if (end == iB || (iA < mid && cmpfunc(source + iA, source + iB) <= 0)) {
			destination[i] = source[iA];
			iA++;
		} else {
			destination[i] = source[iB];
			iB++;
		}
	}
}

void *run_genetic_algorithm(void *arg)
{
	// se preia structura de argumente a functiei
	data variables = *(data *)arg;

	// se stocheaza local toate variabilele date ca argument 
	int thread_id = variables.thread_id;
	sack_object **objects = variables.objects;
	int object_count = variables.object_count;
	int sack_capacity = variables.sack_capacity;
	int generations_count = variables.generations_count;
	int P = variables.P;
	individual **current_generation = variables.current_generation;
	individual **next_generation = variables.next_generation;
	pthread_barrier_t *bariera = variables.bariera;

	int count, cursor;
	individual **tmp = NULL;

	//calcul interval de lucru pentru thread-ul curent
	int start = thread_id * (double)object_count / P;
	int end = fmin((thread_id + 1) * (double)object_count / P, object_count);

	if (thread_id == 0)
	{
		//alocare de memorie
		*current_generation = (individual *)calloc(object_count, sizeof(individual));
		*next_generation = (individual *)calloc(object_count, sizeof(individual));
	}


	// bariera de sincronizare
	pthread_barrier_wait(bariera);

	// alocare de memorie pe intervalul de lucru al thread-ului
	for (int i = start; i < end; ++i)
	{
		(*current_generation)[i].fitness = 0;
		(*current_generation)[i].chromosomes = (int *)calloc(object_count, sizeof(int));
		(*current_generation)[i].chromosomes[i] = 1;
		(*current_generation)[i].index = i;
		(*current_generation)[i].chromosome_length = object_count;

		(*next_generation)[i].fitness = 0;
		(*next_generation)[i].chromosomes = (int *)calloc(object_count, sizeof(int));
		(*next_generation)[i].index = i;
		(*next_generation)[i].chromosome_length = object_count;
	}

	// bariera de sincronizare
	pthread_barrier_wait(bariera);

	for (int k = 0; k < generations_count; ++k)
	{
		cursor = 0;

		// functia compute fitness paralelizata
		compute_fitness_function(*objects, *current_generation, sack_capacity, start, end);

		// sortare interval de lucru al thread-ului
		qsort(*current_generation + start, end - start, sizeof(individual), cmpfunc);

		// bariera de sincronizare
		pthread_barrier_wait(bariera);

		if (thread_id == 0)
		{
			//merge-uire intervale sortate
			{
				individual *auxVec = (individual *)calloc(object_count, sizeof(individual));
				int thread_dreapta = 1;

				while(thread_dreapta < P) {
					int start_sort = thread_dreapta * (double)object_count / P;
					int end_sort = fmin((thread_dreapta + 1) * (double)object_count / P, object_count);

					merge(*current_generation, 0, start_sort, end_sort, auxVec);
					memcpy(*current_generation, auxVec, end_sort * sizeof(individual));
					thread_dreapta++;
				}

				free(auxVec);
			}

			count = object_count * 3 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual((*current_generation) + i, (*next_generation) + i);
			}
			cursor = count;

			count = object_count * 2 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual((*current_generation) + i, (*next_generation) + cursor + i);
				mutate_bit_string_1((*next_generation) + cursor + i, k);
			}
			cursor += count;

			count = object_count * 2 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual((*current_generation) + i + count, (*next_generation) + cursor + i);
				mutate_bit_string_2((*next_generation) + cursor + i, k);
			}
			cursor += count;

			count = object_count * 3 / 10;

			if (count % 2 == 1)
			{
				copy_individual((*current_generation) + object_count - 1, (*next_generation) + cursor + count - 1);
				count--;
			}

			for (int i = 0; i < count; i += 2)
			{
				crossover((*current_generation) + i, (*next_generation) + cursor + i, k);
			}

			tmp = current_generation;
			current_generation = next_generation;
			next_generation = tmp;

			for (int i = 0; i < object_count; ++i)
			{
				(*current_generation)[i].index = i;
			}

			if (k % 5 == 0)
			{
				print_best_fitness(*current_generation);
			}
		}

		// bariera de sincronizare
		pthread_barrier_wait(bariera);
	}

	// functia compute fitness paralelizata
	compute_fitness_function(*objects, *current_generation, sack_capacity, start, end);

	// sortare interval de lucru al thread-ului
	qsort(*current_generation + start, end - start, sizeof(individual), cmpfunc);

	// bariera de sincronizare
	pthread_barrier_wait(bariera);

	if (thread_id == 0)
	{
		//merge-uire intervale sortate
		{
			individual *auxVec = (individual *)calloc(object_count, sizeof(individual));
			int thread_dreapta = 1;

			while(thread_dreapta < P) {
				int start_sort = thread_dreapta * (double)object_count / P;
				int end_sort = fmin((thread_dreapta + 1) * (double)object_count / P, object_count);

				merge(*current_generation, 0, start_sort, end_sort, auxVec);
				memcpy(*current_generation, auxVec, end_sort * sizeof(individual));
				thread_dreapta++;
			}

			free(auxVec);
		}

		print_best_fitness(*current_generation);

		free_generation(*current_generation);
		free_generation(*next_generation);

		free(*current_generation);
		free(*next_generation);
	}

	return 0;
}
