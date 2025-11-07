#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define MAX_ROLLS_TO_PRINT 20
#define DEFAULT_DIE_SIDES 20
#define MAX_HISTOGRAM_AXIS 20
#define BUFFER_CHARS 100

struct ProgramFlags {
	bool less; // true by default, others false
	bool verbose;
	bool histogram;
	bool seen_input;
} flags = {true, false, false, false};

struct DiceRoll {
	char descriptor[BUFFER_CHARS];
	unsigned int len;
	unsigned char sides;
	int rerolls; // positive for advantage, negative for disadvantage
	int modifier;
	int sum;
	double average;
	unsigned char data[];
};

void parse_arg_string(const char *arg, int *len, int *sides, int *rerolls, int *modifier);
struct DiceRoll *convert_arg_to_dice(const char *arg);
unsigned char roll_die(unsigned char sides);
void roll_dice(struct DiceRoll *roll);
void print_rolls(struct DiceRoll *roll);

void parse_arg_string(const char *arg, int *len, int *sides, int *rerolls, int *modifier) {
	char dest[BUFFER_CHARS] = {0};
	strncpy(dest, arg, BUFFER_CHARS-1);
	int i = 0;
	int sign = 1;
	char *len_str = dest;
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
	if (len_str[0]) *len = atoi(len_str);
	else *len = 1; // roll 1 die by default
	if (sides_str != NULL && sides_str[0]) *sides = atoi(sides_str);
	else *sides = DEFAULT_DIE_SIDES; // set to 20 by default
	if (rerolls_str != NULL && rerolls_str[0]) *rerolls = atoi(rerolls_str);
	else *rerolls = 0;
	if (modifier_str != NULL && modifier_str[0]) *modifier = sign * atoi(modifier_str);
	else *modifier = 0;
}

struct DiceRoll *convert_arg_to_dice(const char *arg) {
	struct DiceRoll *roll = NULL;
	int len = 0, sides = 0, rerolls = 0, modifier = 0;
	parse_arg_string(arg, &len, &sides, &rerolls, &modifier);
	if (len <= 0 || sides <= 0) return NULL; // failed arg parsing
	roll = (struct DiceRoll *) calloc(sizeof(struct DiceRoll) + len, 1);
	if (roll == NULL) return NULL; // failed to allocate memory
	int buffered = 0;
	if (modifier) buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i%+i", len, sides, modifier);
	else buffered += snprintf(roll->descriptor, BUFFER_CHARS, "%id%i", len, sides);
	if (rerolls != 0) {
		if (abs(rerolls) > 1) buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, " with x%i ", abs(rerolls));
		else buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, " with ");
		if (rerolls < 0) buffered += snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, "dis");
		snprintf(roll->descriptor+buffered, BUFFER_CHARS-buffered, "advantage");
	}
	roll->len = len;
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
	for (int i = 0; i < roll->len; i++) {
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
		roll->sum += curr_roll;
	}
	roll->average = (double)roll->sum / roll->len;
	roll->sum += roll->modifier;
}

void print_rolls(struct DiceRoll *roll) {
	int *hist = calloc(roll->sides+1, sizeof(int));
	for (int i = 0; i < roll->len; i++) hist[roll->data[i]]++;
	if (flags.less) {
		printf("%d ", roll->sum);
		if (roll->sides == 20 && hist[20] > 0) {
			if (hist[20] == 1) printf("(+1 crit)");
			else printf("(+%d crits)", hist[20]);
		}
		printf("\n");
	}
	if (flags.verbose) {
		printf("rolled %s:\n    data = [", roll->descriptor);
		for (int i = 0; i < roll->len; i++) {
			if (i < MAX_ROLLS_TO_PRINT) printf("%i",roll->data[i]);
			if (i == MAX_ROLLS_TO_PRINT) {
				printf("..."); 
				break;
			}
			if (i < roll->len-1) printf(",");
		}
		printf("]\n    avg = %.3f\n", roll->average);
		if (hist[1])  printf("    nat1 = %d\n", hist[1]);
		if (hist[20]) printf("    nat20 = %d\n", hist[20]);
		printf("    sum = %d (%c%d)\n", roll->sum - roll->modifier, (roll->modifier < 0) ? '-' : '+', roll->modifier);
	}
	if (flags.histogram) {
		const char *BAR = "##################################################";
		const int BAR_LEN = 50;
		int MAX = hist[1];
		for (int i = 2; i <= roll->sides; i++) MAX = (MAX < hist[i]) ? hist[i] : MAX;
		int width = 1;
		while (pow(10, width) <= MAX) width++;
		int axis_width = 1;
		while (pow(10, axis_width) <= roll->sides) axis_width++;
		printf("    hist = [\n");
		for (int i = 1; i <= roll->sides; i++) {
			if (hist[i] != 0 || roll->sides <= MAX_HISTOGRAM_AXIS)
				printf("        %*dx %*d: %.*s\n", width, hist[i], axis_width, i, BAR_LEN*hist[i]/MAX, BAR);
			if (i == roll->sides) printf("    ]\n");
		}
	}
	free(hist);
	}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: error: no input provided\n", argv[0]);
		fprintf(stderr, "usage: roll [-vh] dice ... \n");
		return EXIT_FAILURE;
	}
	int running_total = 0;
	srand(time(NULL));
	struct DiceRoll *roll = NULL;
	if (argv[0][strlen(argv[0])-1] == 'v')
		flags.less = false, flags.verbose = true, flags.histogram = false;
	else if (argv[0][strlen(argv[0])-1] == 'h')
		flags.less = false, flags.verbose = true, flags.histogram = true;
	int j = 1; // tracks flags
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][j] != '\0') {
			while (argv[i][j] != '\0') {
				switch (argv[i][j]) {
				case 'l':
					flags.less = true;
					flags.verbose = false;
					break;
				case 'v':
					flags.less = false;
					flags.verbose = true;
					break;
				case 'h':
					flags.histogram = true;
					break;
				case 'n':
					flags.histogram = false;
					break;
				default:
					fprintf(stderr, "%s: error: illegal option %c\n", argv[0], argv[i][j]);
					return EXIT_FAILURE;
				}
				j++;
			}
		} else {
			roll = convert_arg_to_dice(argv[i]);
			if (roll == NULL) {
				fprintf(stderr, "%s: error: failed to parse arg %s\n", argv[0], argv[i]);
				return EXIT_FAILURE;
			}
			roll_dice(roll);
			print_rolls(roll);
			running_total += roll->sum;
			free(roll);
			roll = NULL;
			if (!flags.seen_input) flags.seen_input = true;
		}
	}
	if (!flags.seen_input) {
		fprintf(stderr, "usage: roll [-vh] dice ... \n");
		return EXIT_FAILURE;
	}
	if (!flags.less) printf("total = %d\n", running_total);
	else if (argc-j+1 > 2) printf("%d\n", running_total);
	return EXIT_SUCCESS;
}
