#include <stdlib.h>
#include <stdio.h>
#include "genetic_algorithm_par.h"
#include <pthread.h>

int main(int argc, char *argv[])
{
	sack_object *objects = NULL;

	int object_count = 0;

	int sack_capacity = 0;

	int generations_count = 0;

	int P; //numbers of thread
	pthread_barrier_t bariera; //bariera pentru sincronizare

	// generatiile de indivizi
	individual *current_generation;
	individual *next_generation;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &P, argc, argv))
	{
		return 0;
	}

	pthread_barrier_init(&bariera, NULL, P);

	pthread_t threads[P]; //threads
	data variables[P]; //vector pentru a transmite argumentele

	//se creaza thread-urile si se apeleaza functia
	for (int i = 0; i < P; i++)
	{
		//se pun variabilele in structura
		variables[i].bariera = &bariera;
		variables[i].current_generation = &current_generation;
		variables[i].next_generation = &next_generation;
		variables[i].generations_count = generations_count;
		variables[i].sack_capacity = sack_capacity;
		variables[i].P = P;
		variables[i].object_count = object_count;
		variables[i].objects = &objects;
		variables[i].thread_id = i;

		//se porneste thread-ul
		int r = pthread_create(&threads[i], NULL, run_genetic_algorithm, &variables[i]);

		if (r)
		{
			exit(-1);
		}
	}

	// se opresc thread-urile
	for (int i = 0; i < P; i++)
	{
		int r = pthread_join(threads[i], NULL);

		if (r)
		{
			exit(-1);
		}
	}

	pthread_barrier_destroy(&bariera);

	free(objects);

	return 0;
}
