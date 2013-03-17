/*
 * Alternate global C function prototypes for Mathomatic.
 *
 * Copyright (C) 1987-2012 George Gesslein II.
 */

/* command function list */
int		clear_cmd(), quit_cmd(), list_cmd(), simplify_cmd(), help_cmd(), eliminate_cmd();
int		fraction_cmd(), unfactor_cmd(), compare_cmd(), extrema_cmd();
int		read_cmd(), display_cmd(), calculate_cmd(), solve_cmd();
int		factor_cmd(), derivative_cmd(), replace_cmd(), approximate_cmd();
int		save_cmd(), taylor_cmd(), limit_cmd(), echo_cmd(), plot_cmd();
int		copy_cmd(), divide_cmd(), pause_cmd(), version_cmd();
int		edit_cmd(), real_cmd(), imaginary_cmd(), tally_cmd();
int		roots_cmd(), set_cmd(), variables_cmd(), code_cmd(), optimize_cmd(), push_cmd(), push_en();
int		sum_cmd(), product_cmd(), for_cmd(), integrate_cmd(), nintegrate_cmd(), laplace_cmd();

/* various functions that don't return int */
char		*dirname_win();
char		*skip_space(), *skip_comma_space(), *skip_param();
char		*get_string();
char		*parse_equation(), *parse_section(), *parse_var(), *parse_var2(), *parse_expr();
char		*list_expression(), *list_equation(), *flist_equation_string();
double		gcd(), gcd_verified(), my_round(), multiply_out_unique();
long		decstrtol(), max_memory_usage();

void fphandler(int sig);
void inthandler(int sig);
void alarmhandler(int sig);
void exithandler(int sig);
void resizehandler(int sig);
