/*
 * Mathomatic symbolic solve routines.
 *
 * Copyright (C) 1987-2012 George Gesslein II.
 
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

The chief copyright holder can be contacted at gesslein@mathomatic.org, or
George Gesslein II, P.O. Box 224, Lansing, NY  14882-0224  USA.
 
 */

#include "includes.h"

#define	MAX_RAISE_POWER	20	/* Maximum number of times to increase power in solve function. */

static int increase(double d, long v);
static int poly_solve(long v);
static int g_of_f(int op, token_type *operandp, token_type *side1p, int *side1np, token_type *side2p, int *side2np);
static int flip(token_type *side1p, int *side1np, token_type *side2p, int *side2np);

static int	repeat_count;
static int	prev_n1, prev_n2;

static int	last_int_var = 0;

/*
 * Solve using equation spaces.  Almost always displays a message.
 *
 * Return true if successful.  If successful, you should display the solve result.
 * You are allowed to simplify the result before display however,
 * preferably with "simplify quick".  Just plain "simplify" expands too much sometimes.
 */
int
solve_espace(want, have)
int	want;	/* equation number containing what to solve for */
int	have;	/* equation number to solve */
{
	int	i;
	jmp_buf	save_save;
	int	rv = 0;		/* solve_sub() return value */

	if (want == have || !equation_space_is_equation(have)) {
#if	LIBRARY || !HELP
		error(_("Solving requires an equation."));
#else
		error(_("Please enter an equation to solve, or a command like \"help\"."));
#endif
		printf(_("Solve failed for equation space #%d.\n"), have + 1);
		return false;
	}
	blt(save_save, jmp_save, sizeof(jmp_save));
	if ((i = setjmp(jmp_save)) != 0) {	/* trap errors */
		clean_up();
		if (i == 14) {
			error(_("Expression too large."));
		}
		rv = 0;
	} else {
		if (n_lhs[want]) {
			if (n_rhs[want]) {
				/* Something in the LHS and RHS of equation number "want". */
				error(_("Can only solve for a single variable or for 0, possibly raised to a power."));
				rv = 0;
			} else {
				/* Normal solve: */
				rv = solve_sub(lhs[want], n_lhs[want], lhs[have], &n_lhs[have], rhs[have], &n_rhs[have]);
			}
		} else {
			/* Solve variable was preceded by an equals sign, solve using reversed equation sides: */
			rv = solve_sub(rhs[want], n_rhs[want], rhs[have], &n_rhs[have], lhs[have], &n_lhs[have]);
		}
	}
	blt(jmp_save, save_save, sizeof(jmp_save));
	if (rv <= 0) {
		printf(_("Solve failed for equation space #%d.\n"), have + 1);
	} else {
		debug_string(0, _("Solve successful:"));
	}
	return(rv > 0);
}

/*
 * Main Mathomatic symbolic solve routine.
 *
 * This works by moving everything containing the variable to solve for
 * to the LHS (via transposition), then moving everything not containing the variable to the
 * RHS.  Many tricks are used, and this routine works very well.
 *
 * Globals tlhs[] and trhs[] are used to hold the actual equation while manipulating.
 *
 * Returns a positive integer if successful, with the result placed in the passed LHS and RHS.
 * Returns 1 for normal success.
 * Returns 2 if successful and a solution was zero and removed.
 * Returns 0 on failure.  Might succeed at a numeric solve.
 * Returns -1 if solving for a variable and the equation is an identity.
 * Returns -2 if unsolvable in all realms.
 */
int
solve_sub(wantp, wantn, leftp, leftnp, rightp, rightnp)
token_type	*wantp;		/* expression to solve for */
int		wantn;		/* length of expression to solve for */
token_type	*leftp;		/* LHS of equation */
int		*leftnp;	/* pointer to length of LHS */
token_type	*rightp;	/* RHS of equation */
int		*rightnp;	/* pointer to length of RHS */
{
	int		i, j;
	int		found, found_count;
	int		worked;
	int		diff_sign;
	int		op, op_kind;
	token_type	*p1, *b1, *ep;
	long		v = 0;			/* variable to solve for */
	int		need_flip;
	int		uf_flag = false;	/* unfactor flag */
	int		qtries = 0;
	int		zflag;			/* true if RHS is currently zero */
	int		zsolve;			/* true if we are solving for zero */
	int		inc_count = 0;
	int		zero_solved = false;
	double		numerator, denominator;
	int		success = 1;

	repeat_count = 0;
	prev_n1 = 0;
	prev_n2 = 0;
	if (*leftnp <= 0 || *rightnp <= 0) {
#if	LIBRARY || !HELP
		error(_("Solving requires an equation."));
#else
		error(_("Please enter an equation to solve, or a command like \"help\"."));
#endif
		return false;
	}
	if (wantn != 1) {
		if (wantn == 3 && wantp[1].token.operatr == POWER
		    && wantp[2].kind == CONSTANT && wantp[2].token.constant > 0.0 && wantp[2].token.constant != 1.0) {
/*
 * Solving for 0^2 will isolate the square root and then square both sides of an equation;
 * and solving for variable^2 will isolate the square root of that variable
 * and then square both sides of the equation.  Works for any power and variable.
 */
			if (wantp[0].kind == VARIABLE) {
				v = wantp[0].token.variable;
			}
			if (solve_sub(&zero_token, 1, rightp, rightnp, leftp, leftnp) <= 0)
				return false;
			n_tlhs = *leftnp;
			blt(tlhs, leftp, n_tlhs * sizeof(*leftp));
			n_trhs = *rightnp;
			blt(trhs, rightp, n_trhs * sizeof(*rightp));
			uf_simp(tlhs, &n_tlhs);
			if (increase(1 / wantp[2].token.constant, v) != true) {
				error(_("Unable to isolate root."));
				return false;
			}
			list_tdebug(2);
			mid_simp_side(tlhs, &n_tlhs);
			simp_loop(trhs, &n_trhs);
			uf_simp(trhs, &n_trhs);
			list_tdebug(1);

			blt(leftp, tlhs, n_tlhs * sizeof(*leftp));
			*leftnp = n_tlhs;
			blt(rightp, trhs, n_trhs * sizeof(*rightp));
			*rightnp = n_trhs;
			return true;
		}
		error(_("Can only solve for a single variable or for 0, possibly raised to a power."));
		return false;
	}
	/* copy the equation to temporary storage where it will be manipulated */
	n_tlhs = *leftnp;
	blt(tlhs, leftp, n_tlhs * sizeof(*leftp));
	n_trhs = *rightnp;
	blt(trhs, rightp, n_trhs * sizeof(*rightp));

	if (wantp->kind == VARIABLE) {
		v = wantp->token.variable;
		if (!found_var(trhs, n_trhs, v) && !found_var(tlhs, n_tlhs, v)) {
			/* variable v is 0 or not found */
			error(_("Solve variable not found."));
			return false;
		}
		zsolve = false;
	} else {
		v = 0;
		if (wantp->kind != CONSTANT || wantp->token.constant != 0.0) {
			error(_("Can only solve for a single variable or for 0, possibly raised to a power."));
			return false;
		}
		debug_string(1, _("Solving for zero..."));
		zsolve = true;
	}
	uf_power(tlhs, &n_tlhs);
	uf_power(trhs, &n_trhs);
simp_again:
	/* Make sure equation is a bit simplified. */
	list_tdebug(2);
	simps_side(tlhs, &n_tlhs, zsolve);
	if (uf_flag) {
		simp_loop(trhs, &n_trhs);
		uf_simp(trhs, &n_trhs);
		factorv(trhs, &n_trhs, v);
	} else {
		simps_side(trhs, &n_trhs, zsolve);
	}
	list_tdebug(1);
no_simp:
	/* First selectively move sub-expressions from the RHS to the LHS. */
	op = 0;
	ep = &trhs[n_trhs];
	if (zsolve) {
		for (b1 = p1 = trhs; p1 < ep; p1++) {
			if (p1->level == 1 && p1->kind == OPERATOR) {
				op = p1->token.operatr;
				b1 = p1 + 1;
				if (op == DIVIDE) {
					if (!g_of_f(op, b1, trhs, &n_trhs, tlhs, &n_tlhs))
						return false;
					goto simp_again;
				}
			}
		}
	} else {
		for (b1 = p1 = trhs; p1 < ep; p1++) {
			if (p1->kind == VARIABLE && v == p1->token.variable) {
				if (op == 0) {
					for (p1++;; p1++) {
						if (p1 >= ep) {
							op = PLUS;
							break;
						}
						if (p1->level == 1 && p1->kind == OPERATOR) {
							switch (p1->token.operatr) {
							case TIMES:
							case DIVIDE:
								op = TIMES;
								break;
							case PLUS:
							case MINUS:
								op = PLUS;
								break;
							default:
								op = p1->token.operatr;
								break;
							}
							break;
						}
					}
				}
				switch (op) {
				case TIMES:
				case DIVIDE:
				case POWER:
					b1 = trhs;
					op = PLUS;
					for (p1 = b1; p1 < ep; p1++)
						p1->level++;
					break;
				}
				if (!g_of_f(op, b1, trhs, &n_trhs, tlhs, &n_tlhs))
					return false;
				goto simp_again;
			} else if (p1->level == 1 && p1->kind == OPERATOR) {
				op = p1->token.operatr;
				b1 = p1 + 1;
			}
		}
	}
	if (uf_flag) {
		simps_side(trhs, &n_trhs, zsolve);
	}
left_again:
	worked = true;
	uf_flag = false;
see_work:
	if (found_var(trhs, n_trhs, v)) {
		/* solve variable in RHS */
		debug_string(1, _("Solve variable moved back to RHS, quitting solve routine."));
		return false;
	}
	/* See if we have solved the equation. */
	if (se_compare(wantp, wantn, tlhs, n_tlhs, &diff_sign) && !diff_sign) {
		if (zsolve) {
			debug_string(1, "Simplifying the zero solve until there are no more divides:");
zero_simp:
			list_tdebug(2);
			uf_power(trhs, &n_trhs);
			do {
				do {
					simp_ssub(trhs, &n_trhs, 0L, 0.0, false, true, 4);
				} while (uf_power(trhs, &n_trhs));
			} while (super_factor(trhs, &n_trhs, 1));
			list_tdebug(1);
			ep = &trhs[n_trhs];
			op = 0;
			for (p1 = trhs + 1; p1 < ep; p1 += 2) {
				if (p1->level == 1) {
					op = p1->token.operatr;
					if (op == DIVIDE) {
						goto no_simp;
					}
					if (op != TIMES) {
						break;
					}
				}
			}
			switch (op) {
			case TIMES:
				for (p1 = trhs; p1 < ep;) {
					b1 = p1;
					for (;; p1++) {
						if (p1 >= ep || (p1->kind == OPERATOR && p1->level == 1)) {
							blt(b1 + 1, p1, (char *) ep - (char *) p1);
							n_trhs -= p1 - (b1 + 1);
							*b1 = one_token;
							goto zero_simp;
						}
						if (p1->kind != CONSTANT && p1->kind != OPERATOR
						    && (p1->kind != VARIABLE || (p1->token.variable & VAR_MASK) > SIGN)) {
							break;
						}
					}
					p1 = b1;
					for (p1++; p1 < ep && p1->level > 1; p1 += 2)
						;
#if	DEBUG
					if (p1 != ep && (p1->kind != OPERATOR || p1->token.operatr != TIMES)) {
						error_bug("Operator mix up in zero_simp.");
					}
#endif
					if ((p1 - 2) > b1) {
						p1 -= 2;
						if (p1->token.operatr == POWER && p1->level == 2
						    /* && ((p1 - 2) <= b1 || (p1 - 2)->token.operatr != POWER || (p1 - 2)->level != 3) */) {
							p1++;
							if (p1->level == 2 && p1->kind == CONSTANT && p1->token.constant > 0.0) {
								p1->token.constant = 1.0;
								goto zero_simp;
							}
							p1++;
						} else
							p1 += 2;
					}
					p1++;
				}
				break;
			case POWER:
/*				if ((p1 - 2) <= trhs || (p1 - 2)->token.operatr != POWER || (p1 - 2)->level != 2) { */
					p1++;
					if (p1->level == 1 && p1->kind == CONSTANT && p1->token.constant > 0.0) {
						n_trhs -= 2;
						goto zero_simp;
					}
/*				} */
				break;
			}
			debug_string(1, _("Solve for zero completed:"));
		} else {
			debug_string(1, _("Solve completed:"));
		}
		list_tdebug(1);
		blt(leftp, tlhs, n_tlhs * sizeof(*leftp));
		*leftnp = n_tlhs;
		blt(rightp, trhs, n_trhs * sizeof(*rightp));
		*rightnp = n_trhs;
		return success;
	}
	/* Move what we don't want in the LHS to the RHS. */
	found_count = 0;
	need_flip = 0;
	found = 0;
	op = 0;
	ep = &tlhs[n_tlhs];
	for (b1 = p1 = tlhs;; p1++) {
		if (p1 >= ep || (p1->level == 1 && p1->kind == OPERATOR)) {
			if (!found) {
				if ((p1 < ep || found_count || zsolve || n_tlhs > 1 || tlhs[0].kind != CONSTANT)
				    && (p1 - b1 != 1 || b1->kind != CONSTANT || b1->token.constant != 1.0
				    || p1 >= ep || p1->token.operatr != DIVIDE)) {
					if (op == 0) {
						for (;; p1++) {
							if (p1 >= ep) {
								op = PLUS;
								break;
							}
							if (p1->level == 1 && p1->kind == OPERATOR) {
								switch (p1->token.operatr) {
								case TIMES:
								case DIVIDE:
									op = TIMES;
									break;
								case PLUS:
								case MINUS:
									op = PLUS;
									break;
								default:
									op = p1->token.operatr;
									break;
								}
								break;
							}
						}
					}
					if (zsolve) {
						if (p1 < ep) {
							switch (op) {
							case PLUS:
							case MINUS:
							case DIVIDE:
								break;
							default:
								goto fin1;
							}
						} else {
							if (op != DIVIDE) {
								b1 = tlhs;
								op = PLUS;
								for (p1 = b1; p1 < ep; p1++)
									p1->level++;
							}
						}
					}
					if (!g_of_f(op, b1, tlhs, &n_tlhs, trhs, &n_trhs))
						return false;
					list_tdebug(2);
					if (uf_flag) {
						simp_loop(tlhs, &n_tlhs);
					} else {
						simps_side(tlhs, &n_tlhs, zsolve);
					}
					simps_side(trhs, &n_trhs, zsolve);
					list_tdebug(1);
					goto see_work;
				}
			} else if (op == DIVIDE) {
				need_flip += found;
			}
			if (p1 >= ep) {
				if (found_count == 0) { /* if solve variable no longer in LHS */
					if (found_var(trhs, n_trhs, v)) {
						/* solve variable in RHS */
						debug_string(1, _("Solve variable moved back to RHS, quitting solve routine."));
						return false;
					}
					/* The following code determines if we have an identity: */
					calc_simp(tlhs, &n_tlhs);
					calc_simp(trhs, &n_trhs);
					if (se_compare(tlhs, n_tlhs, trhs, n_trhs, &diff_sign) && !diff_sign) {
						error(_("This equation is an identity."));
						debug_string(0, _("That is, the LHS is identical to the RHS."));
						return -1;
					}
					found = false;
					for (i = 0; i < n_tlhs; i += 2) {
						if (tlhs[i].kind == VARIABLE && tlhs[i].token.variable > IMAGINARY) {
							found = true;
							break;
						}
					}
					for (i = 0; i < n_trhs; i += 2) {
						if (trhs[i].kind == VARIABLE && trhs[i].token.variable > IMAGINARY) {
							found = true;
							break;
						}
					}
					if (found) {
						error(_("This equation is independent of the solve variable."));
					} else {
						error(_("There are no possible values for the solve variable."));
					}
					return -2;
				}
				zflag = (n_trhs == 1 && trhs[0].kind == CONSTANT && trhs[0].token.constant == 0.0);
				if (zflag) {
					/* overwrite -0.0 */
					trhs[0].token.constant = 0.0;
				}
				if (need_flip >= found_count) {
					if (!flip(tlhs, &n_tlhs, trhs, &n_trhs))
						return false;
					list_tdebug(2);
					simps_side(tlhs, &n_tlhs, zsolve);
					simps_side(trhs, &n_trhs, zsolve);
					list_tdebug(1);
					goto left_again;
				}
				if (worked && !uf_flag) {
					worked = false;
					debug_string(1, _("Unfactoring..."));
					partial_flag = false;
					uf_simp(tlhs, &n_tlhs);
					partial_flag = true;
					factorv(tlhs, &n_tlhs, v);
					list_tdebug(1);
					uf_flag = true;
					goto see_work;
				}
				if (uf_flag) {
					simps_side(tlhs, &n_tlhs, zsolve);
					uf_flag = false;
					goto see_work;
				}
				op = 0;
				b1 = tlhs;
				for (i = 1; i < n_tlhs; i += 2) {
					if (tlhs[i].level == 1) {
						op_kind = tlhs[i].token.operatr;
						if (op_kind == TIMES || op_kind == DIVIDE) {
							if (op == 0) {
								op = TIMES;
							}
						} else {
							op = op_kind;
							break;
						}
						if (zflag) {
							if (op_kind == DIVIDE
							    || (tlhs[i+1].kind == VARIABLE && tlhs[i+1].token.variable == v
							    && (tlhs[i+1].level == 1
							    || (tlhs[i+1].level == 2 && tlhs[i+2].token.operatr == POWER
							    && tlhs[i+3].level == 2 && tlhs[i+3].kind == CONSTANT && tlhs[i+3].token.constant > 0.0)))) {
								op = op_kind;
								b1 = &tlhs[i+1];
								if (op_kind == DIVIDE)
									break;
							}
						} else {
							if (op_kind == DIVIDE) {
								for (j = i + 2; j < n_tlhs && tlhs[j].level > 1; j += 2) {
									if (tlhs[j].level == 2) {
										op_kind = tlhs[j].token.operatr;
										if (op_kind == PLUS || op_kind == MINUS) {
											op = DIVIDE;
											b1 = &tlhs[i+1];
										}
										break;
									}
								}
							}
						}
					}
				}
				if ((zflag && zero_solved && op == TIMES
				    && b1[0].kind == VARIABLE && b1[0].token.variable == v
				    && (b1[0].level == 1 || (b1[0].level == 2 && b1[1].token.operatr == POWER
				    && b1[2].level == 2 && b1[2].kind == CONSTANT && b1[2].token.constant > 0.0)))
				    || op == DIVIDE) {
					if (op == TIMES) {
						qtries = 0;	/* might be quadratic after removing solution */
						success = 2;
#if	!SILENT
						fprintf(gfp, _("Removing possible solution: \""));
						list_proc(b1, 1, false);
						fprintf(gfp, " = 0\".\n");
#endif
					} else {
						debug_string(1, _("Juggling..."));
						uf_flag = true;
					}
					if (!g_of_f(op, b1, tlhs, &n_tlhs, trhs, &n_trhs))
						return false;
					goto simp_again;
				}
				b1 = NULL;
				for (i = 1; i < n_tlhs; i += 2) {
					if (tlhs[i].token.operatr == POWER
					    && tlhs[i+1].level == tlhs[i].level
					    && tlhs[i+1].kind == CONSTANT
					    && fabs(tlhs[i+1].token.constant) < 1.0) {
						if (!f_to_fraction(tlhs[i+1].token.constant, &numerator, &denominator)
						    || fabs(numerator) != 1.0 || denominator < 2.0) {
							continue;
						}
						for (j = i - 1; j >= 0 && tlhs[j].level >= tlhs[i].level; j--) {
							if (tlhs[j].kind == VARIABLE && tlhs[j].token.variable == v) {
								if (b1) {
									if (fabs(b1->token.constant) < fabs(tlhs[i+1].token.constant)) {
										b1 = &tlhs[i+1];
									}
								} else {
									b1 = &tlhs[i+1];
								}
								break;
							}
						}
					}
				}
				if (b1 && zero_solved) {
					inc_count++;
					if (inc_count > MAX_RAISE_POWER)
						return false;
					zero_solved = false;
					qtries = 0;
					if (!increase(b1->token.constant, v)) {
						return false;
					}
					uf_flag = true;
					goto simp_again;
				}
				if (qtries) {
					return false;
				}
				*leftnp = n_tlhs;
				blt(leftp, tlhs, n_tlhs * sizeof(*leftp));
				*rightnp = n_trhs;
				blt(rightp, trhs, n_trhs * sizeof(*rightp));
				if (solve_sub(&zero_token, 1, leftp, leftnp, rightp, rightnp) <= 0)
					return false;
				if (zero_solved) {
					qtries++;
				}
				zero_solved = true;
				if (poly_solve(v)) {
					goto left_again;
				} else {
					goto simp_again;
				}
			} else {
fin1:
				found = 0;
				op = p1->token.operatr;
				b1 = p1 + 1;
			}
		} else if (p1->kind == VARIABLE) {
			if (v == p1->token.variable) {
				found_count++;
				found++;
			}
		}
	}
}

/*
 * Isolate (solve for) the expression containing variable "v" raised to the power of "d",
 * then raise both sides of the equation to the power of 1/d.
 *
 * Return true if successful.
 */
static int
increase(d, v)
double	d;
long	v;
{
	int		flag, foundp, found2;
	int		len1, len2;
	int		op;
	token_type	*b1, *p1, *p2;
	token_type	*ep;

#if	!SILENT
	if (debug_level >= 0) {
		fprintf(gfp, _("Raising both equation sides to the power of %.*g and expanding...\n"), precision, 1.0 / d);
	}
#endif
	list_tdebug(2);
	partial_flag = false;
	ufactor(tlhs, &n_tlhs);
	partial_flag = true;
/*	symb_flag = symblify; */
	simp_ssub(tlhs, &n_tlhs, v, d, true, false, 2);
	simp_ssub(tlhs, &n_tlhs, 0L, 1.0, true, true, 2);
/*	symb_flag = false; */
	list_tdebug(1);

isolate:
	ep = &tlhs[n_tlhs];
	len2 = len1 = 0;
	foundp = false;
	for (p1 = tlhs + 1;; p1 += 2) {
		if (p1 >= ep) {
			return 2;	/* power not found */
		}
		if (p1->level == 1) {
			break;
		}
		if (p1->token.operatr == POWER
		    && (p1 + 1)->level == p1->level
		    && (p1 + 1)->kind == CONSTANT
		    && (p1 + 1)->token.constant == d) {
			flag = false;
			for (b1 = p1 - 1;; b1--) {
				if (b1->level < p1->level) {
					b1++;
					break;
				}
				if (b1->kind == VARIABLE && b1->token.variable == v) {
					flag = true;
				}
				if (b1 == tlhs)
					break;
			}
			if (flag || v == 0) {
				foundp = true;
				len1 = max(len1, p1 - b1);
			}
		}
	}
	found2 = false;
	for (p2 = p1 + 2;; p2 += 2) {
		if (p2 >= ep) {
			break;
		}
		if (p2->token.operatr == POWER
		    && (p2 + 1)->level == p2->level
		    && (p2 + 1)->kind == CONSTANT
		    && (p2 + 1)->token.constant == d) {
			flag = false;
			for (b1 = p2 - 1;; b1--) {
				if (b1->level < p2->level) {
					b1++;
					break;
				}
				if (b1->kind == VARIABLE && b1->token.variable == v) {
					flag = true;
				}
				if (b1 == tlhs)
					break;
			}
			if (flag || v == 0) {
				found2 = true;
				len2 = max(len2, p2 - b1);
			}
		}
	}
	if (foundp && found2) {
		if (len2 > len1)
			foundp = false;
	}
	b1 = p1 + 1;
	op = p1->token.operatr;
	if (op == POWER && b1->level == 1 && b1->kind == CONSTANT && b1->token.constant == d) {
		return(g_of_f(POWER, b1, tlhs, &n_tlhs, trhs, &n_trhs));
	}
	if (!foundp) {
		b1 = tlhs;
		if (p1 - b1 == 1 && p1->token.operatr == DIVIDE
		    && b1->kind == CONSTANT && b1->token.constant == 1.0) {
			if (!flip(tlhs, &n_tlhs, trhs, &n_trhs))
				return false;
			goto end;
		}
		switch (p1->token.operatr) {
		case TIMES:
		case DIVIDE:
			op = TIMES;
			break;
		case PLUS:
		case MINUS:
			op = PLUS;
			break;
		default:
			op = p1->token.operatr;
			break;
		}
	}
	if (!g_of_f(op, b1, tlhs, &n_tlhs, trhs, &n_trhs))
		return false;
end:
	list_tdebug(2);
	simp_loop(tlhs, &n_tlhs);
	simp_loop(trhs, &n_trhs);
	list_tdebug(1);
	goto isolate;
}

/*
 * Quadratic and biquadratic solve routine.
 * Solves any equation of the form "0 = ax^(2n)+bx^n+c" for "x^n",
 * where "x" is an expression containing the solve variable,
 * and "n" is a constant.  Uses the quadratic formula.
 *
 * The equation to solve is in tlhs and trhs, it must already be solved for zero.
 *
 * Return true if successful, with solved equation in tlhs and trhs.
 */
static int
poly_solve(v)
long	v;	/* solve variable */
{
	int		i, j, k;
	token_type	*p1, *p2, *ep;
	token_type	*x1p = NULL, *x2p;
	token_type	*a1p = NULL, *a2p = NULL, *a2ep = NULL;
	token_type	*b1p, *b2p, *b2ep;
	token_type	*x1tp, *a1tp;
	token_type	x1_storage[100];
	int		op, op2, opx1, opx2;
	int		found, diff_sign;
	int		len, alen, blen, aloc, nx1;
	double		high_power = 0.0;

	debug_string(1, _("Checking if equation is a polynomial equation:"));
#if	DEBUG
	if (n_tlhs != 1 || tlhs[0].kind != CONSTANT || tlhs[0].token.constant != 0.0) {
		error_bug("poly_solve() called without a zero-solved equation!");
	}
#endif
	uf_simp(trhs, &n_trhs);
	while (factor_plus(trhs, &n_trhs, v, 0.0)) {
		simp_loop(trhs, &n_trhs);
	}
	list_tdebug(1);

	found = false;
	op = 0;
	ep = &trhs[n_trhs];
	for (x1tp = p1 = trhs;; p1++) {
		if (p1 >= ep || (p1->level == 1 && p1->kind == OPERATOR)) {
			if (p1 < ep) {
				switch (p1->token.operatr) {
				case PLUS:
				case MINUS:
					break;
				default:
					return false;
				}
			}
			if (op == TIMES || op == DIVIDE) {
				found = false;
				op2 = 0;
				for (a1tp = p2 = x1tp;; p2++) {
					if (p2 >= p1)
						break;
					if (p2->level == 2) {
						if (p2->kind == OPERATOR) {
							x1tp = p2 + 1;
							op2 = p2->token.operatr;
							found = false;
						}
					} else {
						if (p2->kind == OPERATOR) {
							if (p2->level == 3 && p2->token.operatr == POWER) {
								if (found && (op2 == TIMES || op2 == 0)
								    && (p2 + 1)->level == 3
								    && (p2 + 1)->kind == CONSTANT
								    && (p2 + 1)->token.constant > high_power) {
									high_power = (p2 + 1)->token.constant;
									x1p = x1tp;
									a1p = a1tp;
									a2p = p2 + 2;
									a2ep = p1;
								}
							}
						} else if (p2->kind == VARIABLE && p2->token.variable == v) {
							found = true;
						}
					}
				}
			} else if (op == POWER && found && (p1 - 1)->level == 2
			    && (p1 - 1)->kind == CONSTANT && (p1 - 1)->token.constant > high_power) {
				high_power = (p1 - 1)->token.constant;
				a1p = x1p = x1tp;
				a2p = p1;
				a2ep = a2p;
			}
			if (p1 >= ep) {
				break;
			}
		}
		if (p1->level == 1) {
			if (p1->kind == OPERATOR) {
				op = 0;
				x1tp = p1 + 1;
				found = false;
			}
		} else {
			if (p1->kind == OPERATOR) {
				if (p1->level == 2) {
					op = p1->token.operatr;
				}
			} else if (op == 0 && p1->kind == VARIABLE && p1->token.variable == v) {
				found = true;
			}
		}
	}
	if (high_power == 0.0)
		return false;
#if	!SILENT
	if (debug_level >= 0) {
		list_var(v, 0);
		fprintf(gfp, _("Equation is a degree %.*g polynomial equation in %s.\n"), precision, high_power, var_str);
	}
#endif
	if (a1p > trhs && (a1p - 1)->token.operatr == MINUS)
		opx1 = MINUS;
	else
		opx1 = PLUS;
	if (high_power == 2.0) {
		nx1 = (a2p - x1p) - 2;
		if (nx1 > ARR_CNT(x1_storage))
			return false;
		blt(x1_storage, x1p, nx1 * sizeof(token_type));
	} else {
		nx1 = (a2p - x1p);
		if (nx1 > ARR_CNT(x1_storage))
			return false;
		blt(x1_storage, x1p, nx1 * sizeof(token_type));
		x1_storage[nx1-1].token.constant /= 2.0;
	}
	opx2 = 0;
	op = 0;
	for (x2p = p1 = trhs;; p1++) {
		if (p1 >= ep || (p1->level == 1 && p1->kind == OPERATOR)) {
			if (se_compare(x1_storage, nx1, x2p, p1 - x2p, &diff_sign)) {
				b1p = x2p;
				b2p = p1;
				b2ep = b2p;
				break;
			}
			if (op == TIMES || op == DIVIDE) {
				op2 = 0;
				for (b1p = p2 = x2p;; p2++) {
					if (p2 >= p1 || (p2->level == 2 && p2->kind == OPERATOR)) {
						if (op2 == 0 || op2 == TIMES) {
							if (se_compare(x1_storage, nx1, x2p, p2 - x2p, &diff_sign)) {
								b2p = p2;
								b2ep = p1;
								goto big_bbreak;
							}
						}
						if (p2 >= p1)
							break;
					}
					if (p2->level == 2 && p2->kind == OPERATOR) {
						x2p = p2 + 1;
						op2 = p2->token.operatr;
					}
				}
			}
			if (p1 >= ep)
				return false;
		}
		if (p1->level == 1) {
			if (p1->kind == OPERATOR) {
				op = 0;
				opx2 = p1->token.operatr;
				x2p = p1 + 1;
			}
		} else {
			if (p1->kind == OPERATOR && p1->level == 2) {
				op = p1->token.operatr;
			}
		}
	}
big_bbreak:
	switch (opx2) {
	case 0:
		opx2 = PLUS;
	case PLUS:
		if (diff_sign)
			opx2 = MINUS;
		break;
	case MINUS:
		if (diff_sign)
			opx2 = PLUS;
		break;
	default:
		return false;
	}
	blt(scratch, b1p, (char *) x2p - (char *) b1p);
	len = x2p - b1p;
	scratch[len].level = 7;
	scratch[len].kind = CONSTANT;
	if (opx2 == MINUS)
		scratch[len].token.constant = -1.0;
	else
		scratch[len].token.constant = 1.0;
	len++;
	blt(&scratch[len], b2p, (char *) b2ep - (char *) b2p);
	len += (b2ep - b2p);
	blen = len;
	j = min_level(scratch, len);
	j = 7 - j;
	for (i = 0; i < len; i++)
		scratch[i].level += j;
	scratch[len].level = 6;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = POWER;
	len++;
	scratch[len].level = 6;
	scratch[len].kind = CONSTANT;
	scratch[len].token.constant = 2.0;
	len++;
	scratch[len].level = 5;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = MINUS;
	len++;
	scratch[len].level = 6;
	scratch[len].kind = CONSTANT;
	scratch[len].token.constant = 4.0;
	len++;
	scratch[len].level = 6;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = TIMES;
	len++;
	aloc = len;
	blt(&scratch[len], a1p, (char *) x1p - (char *) a1p);
	len += (x1p - a1p);
	scratch[len].level = 7;
	scratch[len].kind = CONSTANT;
	if (opx1 == MINUS)
		scratch[len].token.constant = -1.0;
	else
		scratch[len].token.constant = 1.0;
	len++;
	blt(&scratch[len], a2p, (char *) a2ep - (char *) a2p);
	len += (a2ep - a2p);
	alen = len - aloc;
	j = min_level(&scratch[aloc], len - aloc);
	j = 7 - j;
	for (i = aloc; i < len; i++)
		scratch[i].level += j;
	scratch[len].level = 6;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = TIMES;
	len++;
	k = len;
	scratch[len] = zero_token;
	len++;
	for (p2 = p1 = trhs;; p1++) {
		if (p1 >= ep || (p1->level == 1 && p1->kind == OPERATOR)) {
			if (!((p2 <= x1p && p1 > x1p) || (p2 <= x2p && p1 > x2p))) {
				if (p2 == trhs) {
					scratch[len].level = 1;
					scratch[len].kind = OPERATOR;
					scratch[len].token.operatr = PLUS;
					len++;
				}
				blt(&scratch[len], p2, (char *) p1 - (char *) p2);
				len += (p1 - p2);
			}
			if (p1 >= ep)
				break;
			else
				p2 = p1;
		}
	}
	for (i = k; i < len; i++)
		scratch[i].level += 6;
	scratch[len].level = 4;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = POWER;
	len++;
	scratch[len].level = 4;
	scratch[len].kind = CONSTANT;
	scratch[len].token.constant = 0.5;
	len++;
	scratch[len].level = 3;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = TIMES;
	len++;
	scratch[len].level = 3;
	scratch[len].kind = VARIABLE;
	next_sign(&scratch[len].token.variable);
	len++;
	scratch[len].level = 2;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = MINUS;
	len++;
	if (len + blen + 3 + alen > n_tokens) {
		error_huge();
	}
	blt(&scratch[len], scratch, blen * sizeof(token_type));
	len += blen;
	scratch[len].level = 1;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = DIVIDE;
	len++;
	scratch[len].level = 2;
	scratch[len].kind = CONSTANT;
	scratch[len].token.constant = 2.0;
	len++;
	scratch[len].level = 2;
	scratch[len].kind = OPERATOR;
	scratch[len].token.operatr = TIMES;
	len++;
	blt(&scratch[len], &scratch[aloc], alen * sizeof(token_type));
	len += alen;
	if (found_var(scratch, len, v))
		return false;
	blt(tlhs, x1_storage, nx1 * sizeof(token_type));
	n_tlhs = nx1;
	simp_loop(tlhs, &n_tlhs);
	blt(trhs, scratch, len * sizeof(token_type));
	n_trhs = len;
	simp_loop(trhs, &n_trhs);
	list_tdebug(2);
	uf_tsimp(trhs, &n_trhs);	/* don't unfactor result so much, just unfactor what will be unfactored anyway */
	simps_side(trhs, &n_trhs, false);
	list_tdebug(1);
	debug_string(0, _("Equation was solved with the quadratic formula."));
	return true;
}

/*
 * This is the heart of Mathomatic solving:
 * It applies an identical mathematical operation to both sides of an equation.
 *
 * Solving in Mathomatic is almost entirely based on the rule:
 *	y = f(x)
 *	g(y) = g(f(x))
 * where f() and g() are any function, and:
 *	arcf(y) = arcf(f(x))
 *	arcf(y) = x
 * where arcf() is the inverse function of f().
 * An equality will remain an equality
 * when both sides of the equation are operated on by the same mathematical operation.
 * Some simplification is also necessary during solving, though it is not done in this routine.
 *
 * Apply the inverse of the operation "op" followed by expression "operandp",
 * which is somewhere in "side1p", to both sides of an equation,
 * which is "side1p" and "side2p".
 *
 * Return true unless something is wrong.
 */
static int
g_of_f(op, operandp, side1p, side1np, side2p, side2np)
int		op;		/* current operator */
token_type	*operandp;	/* operand pointer */
token_type	*side1p;	/* equation side pointer */
int		*side1np;	/* pointer to the length of "side1p" */
token_type	*side2p;	/* equation side pointer */
int		*side2np;	/* pointer to the length of "side2p" */
{
	token_type	*p1, *p2, *ep;
	int		oldn, operandn;
	double		numerator, denominator;
	double		d1, d2;
	complexs	c1, c2;
	char		var_name_buf[MAX_VAR_LEN];

	oldn = *side1np;
	ep = &side1p[oldn];
	if (operandp < side1p || operandp >= ep) {
		error_bug("g_of_f() called with invalid operandp.");
	}
	if (*side1np == prev_n1 && *side2np == prev_n2) {
		if (++repeat_count >= 4) {
			debug_string(1, _("Infinite loop aborted in solve routine."));
			return false;
		}
	} else {
		prev_n1 = *side1np;
		prev_n2 = *side2np;
		repeat_count = 0;
	}
	switch (op) {
	case PLUS:
	case MINUS:
	case TIMES:
	case DIVIDE:
	case POWER:
	case MODULUS:
		break;
	default:
		return false;
	}
	for (p1 = operandp + 1; p1 < ep; p1 += 2) {
		if (p1->level == 1) {
			switch (p1->token.operatr) {
			case FACTORIAL:
				op = PLUS;
				continue;
			case MODULUS:
				operandp = p1 + 1;
				continue;
			}
			break;
		}
	}
	operandn = p1 - operandp;
	if (op == POWER && operandp == side1p) {
		if (!parse_complex(side2p, *side2np, &c1))
			return false;
		if (!parse_complex(operandp, operandn, &c2))
			return false;
		debug_string(1, _("Taking logarithm of both equation sides:"));
		errno = 0;
		c1 = complex_div(complex_log(c1), complex_log(c2));
		check_err();
		*side2np = 0;
		side2p[*side2np].level = 1;
		side2p[*side2np].kind = CONSTANT;
		side2p[*side2np].token.constant = c1.re;
		(*side2np)++;
		side2p[*side2np].level = 1;
		side2p[*side2np].kind = OPERATOR;
		side2p[*side2np].token.operatr = PLUS;
		(*side2np)++;
		side2p[*side2np].level = 2;
		side2p[*side2np].kind = CONSTANT;
		side2p[*side2np].token.constant = c1.im;
		(*side2np)++;
		side2p[*side2np].level = 2;
		side2p[*side2np].kind = OPERATOR;
		side2p[*side2np].token.operatr = TIMES;
		(*side2np)++;
		side2p[*side2np].level = 2;
		side2p[*side2np].kind = VARIABLE;
		side2p[*side2np].token.variable = IMAGINARY;
		(*side2np)++;

		blt(side1p, p1 + 1, (*side1np - (operandn + 1)) * sizeof(token_type));
		*side1np -= operandn + 1;
		return true;
	}
	if (op == MODULUS) {
		if (get_constant(side2p, *side2np, &d1) && get_constant(operandp, operandn, &d2)) {
			if (fabs(d1) >= fabs(d2)) {
				error(_("There are no possible solutions."));
				return false;
			}
		}
	}
#if	!SILENT
	if (debug_level > 0) {
		switch (op) {
		case PLUS:
			fprintf(gfp, _("Subtracting"));
			break;
		case MINUS:
			fprintf(gfp, _("Adding"));
			break;
		case TIMES:
			fprintf(gfp, _("Dividing both sides of the equation by"));
			break;
		case DIVIDE:
			fprintf(gfp, _("Multiplying both sides of the equation by"));
			break;
		case POWER:
			fprintf(gfp, _("Raising both sides of the equation to the power of"));
			break;
		case MODULUS:
			fprintf(gfp, _("Applying inverse modulus of"));
			break;
		}
		if (op == POWER && operandn == 1 && operandp->kind == CONSTANT) {
			fprintf(gfp, " %.*g:\n", precision, 1.0 / operandp->token.constant);
		} else {
			fprintf(gfp, " \"");
			if (op == POWER)
				fprintf(gfp, "1/(");
			list_proc(operandp, operandn, false);
			switch (op) {
			case PLUS:
				fprintf(gfp, _("\" from both sides of the equation:\n"));
				break;
			case MINUS:
			case MODULUS:
				fprintf(gfp, _("\" to both sides of the equation:\n"));
				break;
			case POWER:
				fprintf(gfp, ")");
			default:
				fprintf(gfp, "\":\n");
				break;
			}
		}
	}
#endif
	if (*side1np + operandn + 3 > n_tokens || *side2np + operandn + 5 > n_tokens) {
		error_huge();
	}
	if (min_level(side1p, oldn) <= 1) {
		for (p2 = side1p; p2 < ep; p2++)
			p2->level++;
	}
	ep = &side2p[*side2np];
	if (min_level(side2p, *side2np) <= 1) {
		for (p2 = side2p; p2 < ep; p2++)
			p2->level++;
	}
	p2 = &side1p[oldn];
	switch (op) {
	case MODULUS:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = PLUS;
		p2++;
		p2->level = 2;
		p2->kind = VARIABLE;
		snprintf(var_name_buf, sizeof(var_name_buf), "%s_any%.0d", V_INTEGER_PREFIX, last_int_var);
		if (parse_var(&p2->token.variable, var_name_buf) == NULL)
			return false;
		last_int_var++;
		if (last_int_var < 0) {
			last_int_var = 0;
		}
		p2++;
		p2->level = 2;
		p2->kind = OPERATOR;
		p2->token.operatr = TIMES;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 3 + operandn;
		break;
	case POWER:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = POWER;
		p2++;
		p2->level = 2;
		p2->kind = CONSTANT;
		p2->token.constant = 1.0;
		p2++;
		p2->level = 2;
		p2->kind = OPERATOR;
		p2->token.operatr = DIVIDE;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 3 + operandn;
		break;
	case TIMES:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = DIVIDE;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 1 + operandn;
		break;
	case DIVIDE:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = TIMES;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 1 + operandn;
		break;
	case PLUS:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = MINUS;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 1 + operandn;
		break;
	case MINUS:
		p2->level = 1;
		p2->kind = OPERATOR;
		p2->token.operatr = PLUS;
		p2++;
		blt(p2, operandp, (char *) p1 - (char *) operandp);
		*side1np += 1 + operandn;
		break;
	}
	blt(&side2p[*side2np], &side1p[oldn], (*side1np - oldn) * sizeof(*side1p));
	*side2np += *side1np - oldn;
	if (op == POWER && operandn == 1 && operandp->kind == CONSTANT) {
		f_to_fraction(operandp->token.constant, &numerator, &denominator);
		if (always_positive(numerator)) {
			ep = &side2p[*side2np];
			for (p2 = side2p; p2 < ep; p2++)
				p2->level++;
			p2->level = 1;
			p2->kind = OPERATOR;
			p2->token.operatr = TIMES;
			p2++;
			p2->level = 1;
			p2->kind = VARIABLE;
			next_sign(&p2->token.variable);
			*side2np += 2;
		}
	}
	if (op == POWER || op == MODULUS) {
		*side1np = (operandp - 1) - side1p;
	}
	return true;
}

/*
 * Take the reciprocal of both equation sides.
 *
 * Return true if successful.
 */
static int
flip(side1p, side1np, side2p, side2np)
token_type	*side1p;	/* equation side pointer */
int		*side1np;	/* pointer to equation side length */
token_type	*side2p;
int		*side2np;
{
	token_type	*p1, *ep;

	debug_string(1, _("Taking the reciprocal of both sides of the equation..."));
	if (*side1np + 2 > n_tokens || *side2np + 2 > n_tokens) {
		error_huge();
	}
	ep = &side1p[*side1np];
	for (p1 = side1p; p1 < ep; p1++)
		p1->level++;
	ep = &side2p[*side2np];
	for (p1 = side2p; p1 < ep; p1++)
		p1->level++;
	blt(side1p + 2, side1p, *side1np * sizeof(*side1p));
	*side1np += 2;
	blt(side2p + 2, side2p, *side2np * sizeof(*side2p));
	*side2np += 2;

	*side1p = one_token;
	side1p++;
	side1p->level = 1;
	side1p->kind = OPERATOR;
	side1p->token.operatr = DIVIDE;

	*side2p = one_token;
	side2p++;
	side2p->level = 1;
	side2p->kind = OPERATOR;
	side2p->token.operatr = DIVIDE;
	return true;
}
