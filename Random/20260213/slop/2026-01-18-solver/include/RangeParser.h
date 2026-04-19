#ifndef RANGE_PARSER_H
#define RANGE_PARSER_H

typedef struct {
    int (*hands)[2];  // Dynamically allocated array
    double *hand_percentages;  // Dynamically allocated array
    int count;
    int capacity;  // Current capacity (grows as needed)
    double percentage;  // Overall opening frequency (0.0 to 1.0), 1.0 = 100%
} HandRange;

HandRange* parse_range(const char *range_str);
void free_range(HandRange *range);
void print_range_summary(const HandRange *range);
const char* hand_category(int c0, int c1);

#endif
