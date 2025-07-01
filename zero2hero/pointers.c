#include <stdio.h>
#include <stdbool.h>

struct employee_t {
	int id;
	int income;
	bool staff;
};

struct employee_t change_employee(struct employee_t Ralph) {
	Ralph.income = 8888;
	return Ralph;
}

int main() {
	struct employee_t Ralph = {0, 8000, true};
	Ralph = change_employee(Ralph);
	printf("%d\n", Ralph.income);
}
