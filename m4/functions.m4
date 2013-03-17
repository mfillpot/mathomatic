set no prompt

; This m4 input file is used by the shell scripts "matho" and "rmath".
; This defines and enables named math functions in Mathomatic.
; Most functions here should be real number, complex number, and symbolically capable.

; m4 macro definitions for some elementary math functions and constants in Mathomatic.
m4_define(`sqrt', `(($1)**.5)'); function sqrt(x) = square root of x
m4_define(`cbrt', `(($1)**(1/3))'); function cbrt(x) = cube root of x
m4_define(`exp', `(e**($1))'); function exp(x) = e^x
m4_define(`pow', `(($1)**($2))'); function pow(x, y) = x^y
m4_define(`abs', `(|($1)|)'); function abs(x) = absolute value = |x|
m4_define(`sgn', `(($1)/|($1)|)'); signum function sgn(x) = sign of x; sgn(0) fails.
m4_define(`factorial', `(($1)!)'); factorial(x) function = x!
m4_define(`gamma', `((($1)-1)!)'); gamma(x) function = (x-1)!
m4_define(`phi', `((1+5**.5)/2)'); phi = the golden ratio constant, a root of x^2-x-1=0
m4_define(`omega', `(0.5671432904097838729999686622)'); an approximation of the Omega constant
m4_define(`euler', `(0.57721566490153286060651209008)'); the Euler-Mascheroni constant (approximation)

;set modulus_mode=2 >/dev/null ; mode 1 or 2 required for floor() and ceil().
m4_define(`floor', `(($1)-($1)%1)'); floor(x) = floor function, real x -> integer result
m4_define(`ceil', `(($1)+(-($1))%1)'); ceil(x) = ceiling function, real x -> integer result
m4_define(`int', `(($1)//1)'); int(x) = truncate to integer, real x -> integer result
m4_define(`round', `((($1)+|($1)|/($1)/2)//1)'); round(x) = round to nearest integer; beware, round(0) fails.

; Standard trigonometry functions as complex exponentials follow.
; Based on Euler's identity: e^(i*x) = cos(x) + i*sin(x)
; Argument x is in radians.
m4_define(`sin', `((e**(i*($1))-e**(-i*($1)))/(2i))'); sin(x) = sine of x
m4_define(`cos', `((e**(i*($1))+e**(-i*($1)))/2)'); cos(x) = cosine of x
m4_define(`tan', `((e**(i*($1))-e**(-i*($1)))/(i*(e**(i*($1))+e**(-i*($1)))))'); tan(x) = tangent of x
m4_define(`cot', `(i*(e**(i*($1))+e**(-i*($1)))/(e**(i*($1))-e**(-i*($1))))'); cot(x) = cotangent of x
m4_define(`sec', `(2/(e**(i*($1))+e**(-i*($1))))'); sec(x) = secant of x
m4_define(`csc', `(2i/(e**(i*($1))-e**(-i*($1))))'); csc(x) = cosecant of x
m4_define(`sinc', `(((e**(i*pi*($1))-e**(-i*pi*($1)))/(2i))/(pi*($1)))'); sinc(x) = normalized sinc function

; Standard hyperbolic trigonometry functions follow.
; Available are sinh(x), cosh(x), tanh(x), coth(x), sech(x), and csch(x).
; These are related to the above trigonometry functions without the "h" appended to the function name.
m4_define(`sinh', `((e**($1)-e**-($1))/2)')
m4_define(`cosh', `((e**($1)+e**-($1))/2)')
m4_define(`tanh', `((e**($1)-e**-($1))/(e**($1)+e**-($1)))')
m4_define(`coth', `((e**($1)+e**-($1))/(e**($1)-e**-($1)))')
m4_define(`sech', `(2/(e**($1)+e**-($1)))')
m4_define(`csch', `(2/(e**($1)-e**-($1)))')

echo Press the EOF character (Control-D) if you wish to exit Mathomatic.
echo Standard functions are now available, except for logarithms.
set prompt >/dev/null
