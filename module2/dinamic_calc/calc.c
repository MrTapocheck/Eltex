#include "needed.h"

#include <math.h>
#include <stdio.h>

#if defined(BUILD_sum) || defined(BUILD_ALL)

double sum(int num_args, const double *values)
{
	double result = 0.0;
	int i;

	for (i = 0; i < num_args; i++)
		result += values[i];

	return result;
}

#endif

#if defined(BUILD_multiply) || defined(BUILD_ALL)

double multiply(int num_args, const double *values)
{
	double result = 1.0;
	int i;

	if (num_args <= 0)
		return 0.0;

	for (i = 0; i < num_args; i++)
		result *= values[i];

	return result;
}

#endif

#if defined(BUILD_max) || defined(BUILD_ALL)

double max(int num_args, const double *values)
{
	double result;
	int i;

	if (num_args <= 0)
		return 0.0;

	result = values[0];
	for (i = 1; i < num_args; i++) {
		if (values[i] > result)
			result = values[i];
	}

	return result;
}

#endif

#if defined(BUILD_min) || defined(BUILD_ALL)

double min(int num_args, const double *values)
{
	double result;
	int i;

	if (num_args <= 0)
		return 0.0;

	result = values[0];
	for (i = 1; i < num_args; i++) {
		if (values[i] < result)
			result = values[i];
	}

	return result;
}

#endif

#if defined(BUILD_divide) || defined(BUILD_ALL)

double divide(int num_args, const double *values)
{
	double result;
	int i;

	if (num_args < 2)
		return NAN;

	result = values[0];
	for (i = 1; i < num_args; i++) {
		if (values[i] == 0.0) {
			fprintf(stderr, "Ошибка: деление на ноль\n");
			return NAN;
		}
		result /= values[i];
	}

	return result;
}

#endif
