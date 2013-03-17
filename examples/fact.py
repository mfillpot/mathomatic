# This is a general factorial function written in Python.
# A factorial is the product of all positive integers <= a given number.
# Works transparently with integers and floating point;
# that is, it returns the same type as its single argument.
# Gives an error for negative or non-integer input values.

def factorial(x):
	"Return x! (x factorial)."
	if (x < 0 or (x % 1.0) != 0.0):
		raise ValueError, "Factorial argument must be a positive integer."
	if (x == 0):
		return x + 1
	d = x
	while (x > 2):
		x -= 1
		temp = d * x
		if (temp <= d):
			raise ValueError, "Factorial result too large."
		d = temp
	return d
