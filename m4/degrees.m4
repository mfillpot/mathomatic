set no prompt
; Read this into rmath with "rmath degrees.m4" to use trig
; with arguments in degree units instead of radians.

; Standard trigonometry functions as complex exponentials follow.
; Argument x is in degrees.
m4_define(`sin', `((e**(i*(($1)*pi/180))-e**(-i*(($1)*pi/180)))/(2i))'); sin(x) = sine of x
m4_define(`cos', `((e**(i*(($1)*pi/180))+e**(-i*(($1)*pi/180)))/2)'); cos(x) = cosine of x
m4_define(`tan', `((e**(i*(($1)*pi/180))-e**(-i*(($1)*pi/180)))/(i*(e**(i*(($1)*pi/180))+e**(-i*(($1)*pi/180)))))'); tan(x) = tangent of x
m4_define(`cot', `(i*(e**(i*(($1)*pi/180))+e**(-i*(($1)*pi/180)))/(e**(i*(($1)*pi/180))-e**(-i*(($1)*pi/180))))'); cot(x) = cotangent of x
m4_define(`sec', `(2/(e**(i*(($1)*pi/180))+e**(-i*(($1)*pi/180))))'); sec(x) = secant of x
m4_define(`csc', `(2i/(e**(i*(($1)*pi/180))-e**(-i*(($1)*pi/180))))'); csc(x) = cosecant of x

echo Trig function arguments are now in degree units only.
set prompt >/dev/null
