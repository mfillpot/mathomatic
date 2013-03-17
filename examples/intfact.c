/*
 * Factorial function in C for positive integers.
 *
 * Return (arg!).
 * Returns -1 on error.
 */
int
factorial(int arg)
{
	int	result;

	if (arg < 0)
		return -1;
	for (result = 1; result > 0 && arg > 1; arg--) {
		result *= arg;
	}
	if (result <= 0)	/* return -1 on overflow */
		return -1;
	return result;
}
