#ifndef NEEDED_H
#define NEEDED_H

#define MAX_OPS		32
#define MAX_ARGS	32
#define MAX_NAME_LEN	32
#define LIBS_DIR	"libs"

typedef double (*operation_func)(int num_args, const double *values);

struct operation {
	char sym[MAX_NAME_LEN];
	char name[MAX_NAME_LEN];
	operation_func func;
	void *handle;
};

struct operation_table {
	struct operation ops[MAX_OPS];
	int count;
};

const char *operation_title(const char *sym);

double sum(int num_args, const double *values);
double multiply(int num_args, const double *values);
double max(int num_args, const double *values);
double min(int num_args, const double *values);
double divide(int num_args, const double *values);

#endif
