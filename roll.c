#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_ROLLS_TO_PRINT 20
#define MAX_HISTOGRAM_AXIS 20
#define BUFFER_CHARS 100

struct DiceRoll {
	char descriptor[BUFFER_CHARS];
	int size;
	int modifier;
	int rerolls; // positive for advantage, negative for disadvantage
	int total;
	double avg;
	unsigned char sides;
	unsigned char data[];
};

void parse_arg_string(const char *arg, int *size, int *sides, int *rerolls, int *modifier);
struct DiceRoll *convert_arg_to_dice(const char *arg);
unsigned char roll_die(unsigned char sides);
void roll_dice(struct DiceRoll *roll);
void print_rolls(struct DiceRoll *roll, char verbose);

void parse_arg_string(const char *arg, int *size, int *sides, int *rerolls, int *modifier) {
	char dest[BUFFER_CHARS] = {0};
	strcpy(dest, arg);
	int i = 0;
	int sign = 1;
	char *size_str = dest;
	char *sides_str = NULL;
	char *rerolls_str = NULL;
	char *modifier_str = NULL;
	while (dest[i] != '\0') {
		if (dest[i] == 'd') {
			dest[i] = '\0';
			sides_str = dest+i+1;
		} else if (dest[i] == '^') {
			dest[i] = '\0';
			rerolls_str = dest+i+1;
		} else if (dest[i] == '+') {
			dest[i] = '\0';
			modifier_str = dest+i+1;
		} else if (dest[i] == '-') {
			if (rerolls_str != dest+i) {
				sign = -1;
				dest[i] = '\0';
				modifier_str = dest+i+1;
			}
		}
		i++;
	}
	if (size_str[0]) *size = atoi(size_str);
	else *size = 1; // roll 1 die by default
	if (sides_str != NULL && sides_str[0]) *sides = atoi(sides_str);
	else *sides = 20; // set to 20 by default
	if (rerolls_str != NULL && rerolls_str[0]) *rerolls = atoi(rerolls_str);
	else *rerolls = 0;
	if (modifier_str != NULL && modifier_str[0]) *modifier = sign * atoi(modifier_str);
	else *modifier = 0;
	return;
}

struct DiceRoll *convert_arg_to_dice(const char *arg) {
	struct DiceRoll *roll = NULL;
	int size = 0, sides = 0, rerolls = 0, modifier = 0;
	parse_arg_string(arg, &size, &sides, &rerolls, &modifier);
	roll = (struct DiceRoll *) calloc(sizeof(struct DiceRoll) + size, 1);
	int buffered = 0;
	if (modifier) buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i%+i", size, sides, modifier);
	else buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i", size, sides);
	if (rerolls != 0) {
		buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, "%s", (rerolls > 0) ? " with advantage" : " with disadvantage");
		if (abs(rerolls) > 1) snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, " x%i", abs(rerolls));
	}
	roll->size = size;
	roll->sides = sides;
	roll->rerolls = rerolls;
	roll->modifier = modifier;
	return roll;
}

unsigned char roll_die(unsigned char sides) {
	const int LIMIT = RAND_MAX - (RAND_MAX % sides);
	int r = RAND_MAX;
	while (r >= LIMIT) r = rand();
	return r % sides +1;
}

void roll_dice(struct DiceRoll *roll) {
	unsigned char curr_roll = 0;
	unsigned char reroll = 0;
	for (int i = 0; i < roll->size; i++) {
		curr_roll = roll_die(roll->sides);
		if (roll->rerolls != 0) {
			int num_rerolls = abs(roll->rerolls);
			for (int j = 0; j < num_rerolls; j++) {
				reroll = roll_die(roll->sides);
				if (roll->rerolls > 0)
					curr_roll = (reroll > curr_roll) ? reroll : curr_roll;
				else
					curr_roll = (reroll < curr_roll) ? reroll : curr_roll;
			}
		}
		roll->data[i] = curr_roll;
		roll->total += curr_roll;
	}
	roll->avg = (double)roll->total / roll->size;
	roll->total += roll->modifier;
}

void print_rolls(struct DiceRoll *roll, char verbose) {
	int *hist = calloc(roll->sides+1, sizeof(int));
	for (int i = 0; i < roll->size; i++) hist[roll->data[i]]++;
	if (verbose == 'l') {
		printf("%d ", roll->total);
		if (roll->sides == 20 && hist[20] > 0) {
			if (hist[20] == 1) printf("(+1 crit)");
			else printf("(+%d crits)", hist[20]);
		}
		printf("\n");
		return;
	}
	printf("rolled %s:\n    data = [", roll->descriptor);
	for (int i = 0; i < roll->size; i++) {
		if (i < MAX_ROLLS_TO_PRINT) printf("%i",roll->data[i]);
		if (i == MAX_ROLLS_TO_PRINT) {
			printf("..."); 
			break;
		}
		if (i < roll->size-1) printf(",");
	}
	printf("]\n    sum = %d\n", roll->total);
	printf("    avg = %.3f\n", roll->avg);
	if (hist[1])  printf("    nat1  = %d\n", hist[1]);
	if (hist[20]) printf("    nat20 = %d\n", hist[20]);
	if (verbose == 'h') {
		const char *bar = "##################################################";
		int max_roll = hist[1];
		for (int i = 2; i <= roll->sides; i++) max_roll = (max_roll < hist[i]) ? hist[i] : max_roll;
		int width = 1;
		while (pow(10, width) <= max_roll) width++;
		int axis_width = 1;
		while (pow(10, axis_width) <= roll->sides) axis_width++;
		printf("    hist = [\n");
		for (int i = 1; i <= roll->sides; i++) {
			if (hist[i] != 0 || roll->sides <= MAX_HISTOGRAM_AXIS)
				printf("        %*dx %*d: %.*s\n", width, hist[i], axis_width, i, 50*hist[i]/max_roll, bar);
			if (i == roll->sides) printf("    ]\n");
		}
	}
	free(hist);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: error: no input provided\n", argv[0]);
		return EXIT_FAILURE;
	}
	int running_total = 0;
	char verbose = argv[0][strlen(argv[0])-1];
	srand(time(NULL));
	struct DiceRoll *roll = NULL;
	for (int i = 1; i < argc; i++) {
		roll = convert_arg_to_dice(argv[i]);
		if (roll == NULL) {
			fprintf(stderr, "%s: error: failed to parse arg %i\n", argv[0], i);
			return EXIT_FAILURE;
		}
		roll_dice(roll);
		print_rolls(roll, verbose);
		running_total += roll->total;
		free(roll);
		roll = NULL;
	}
	if (verbose != 'l') printf("total = %d\n", running_total);
	else if (argc > 2) printf("%d\n", running_total);
	return EXIT_SUCCESS;
}
