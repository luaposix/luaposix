/*
	Mode Muncher -- modemuncher.c
	961110 Claudio Terra

	munch vb
	[ME monchen, perh. influenced by MF mangier to eat --more at MANGER]
	:to chew with a crunching sound: eat with relish
	:to chew food with a crunching sound: eat food with relish
	--munch-er n

	The NeXT Digital Edition of Webster's Ninth New Collegiate Dictionary
	and Webster's Collegiate Thesaurus
*/

/* struct for rwx <-> POSIX constant lookup tables */
static struct {
	char rwx;
	mode_t bits;
} modesel[] =
{
	/* RWX char				POSIX Constant */
	{'r',					S_IRUSR},
	{'w',					S_IWUSR},
	{'x',					S_IXUSR},

	{'r',					S_IRGRP},
	{'w',					S_IWGRP},
	{'x',					S_IXGRP},

	{'r',					S_IROTH},
	{'w',					S_IWOTH},
	{'x',					S_IXOTH},
	{0, 					(mode_t)-1} /* do not delete this line */
};



static int rwxrwxrwx(mode_t *mode, const char *p)
{
	int count;
	mode_t tmp_mode = *mode;

	tmp_mode &= ~(S_ISUID | S_ISGID); /* turn off suid and sgid flags */
	for (count=0; count<9; count ++)
	{
		if (*p == modesel[count].rwx)
			tmp_mode |= modesel[count].bits;	/* set a bit */
		else if (*p == '-')
			tmp_mode &= ~modesel[count].bits;	/* clear a bit */
		else if (*p=='s')
			switch(count)
			{
			case 2: /* turn on suid flag */
				tmp_mode |= S_ISUID | S_IXUSR;
				break;

			case 5: /* turn on sgid flag */
				tmp_mode |= S_ISGID | S_IXGRP;
				break;

			default:
				return -4; /* failed! -- bad rwxrwxrwx mode change */
				break;
			}
		p++;
	}
	*mode = tmp_mode;
	return 0;
}

static int mode_munch(mode_t *mode, const char* p)
{

	char op=0;
	mode_t affected_bits, ch_mode;
	int done = 0;

	while (!done)
	{
		/* step 0 -- clear temporary variables */
		affected_bits=0;
		ch_mode=0;

		/* step 1 -- who's affected? */

		/* mode string given in rwxrwxrwx format */
		if (*p== 'r' || *p == '-')
			return rwxrwxrwx(mode, p);

		/* mode string given in ugoa+-=rwx format */
		for ( ; ; p++)
			switch (*p)
			{
				case 'u':
				affected_bits |= 04700;
				break;

				case 'g':
				affected_bits |= 02070;
				break;

				case 'o':
				affected_bits |= 01007;
				break;

				case 'a':
				affected_bits |= 07777;
				break;

				/* ignore spaces */
				case ' ':
				break;


				default:
				goto no_more_affected;
			}

		no_more_affected:
		/* If none specified, affect all bits. */
		if (affected_bits == 0)
			affected_bits = 07777;

		/* step 2 -- how is it changed? */
		switch (*p)
		{
			case '+':
			case '-':
			case '=':
			op = *p;
			break;

			/* ignore spaces */
			case ' ':
			break;

			default:
			return -1; /* failed! -- bad operator */
		}

		/* step 3 -- what are the changes? */
		for (p++ ; *p!=0 ; p++)
			switch (*p)
			{
			case 'r':
				ch_mode |= 00444;
				break;

			case 'w':
				ch_mode |= 00222;
				break;

			case 'x':
				ch_mode |= 00111;
				break;

			case 's':
				/* Set the setuid/gid bits if `u' or `g' is selected. */
				ch_mode |= 06000;
				break;

			case ' ':
				/* ignore spaces */
				break;

			default:
				goto specs_done;
			}

		specs_done:
		/* step 4 -- apply the changes */
		if (*p != ',')
			done = 1;
		if (*p != 0 && *p != ' ' && *p != ',')
			return -2; /* failed! -- bad mode change */
		p++;
		if (ch_mode)
			switch (op)
			{
			case '+':
				*mode = *mode |= ch_mode & affected_bits;
				break;

			case '-':
				*mode = *mode &= ~(ch_mode & affected_bits);
				break;

			case '=':
				*mode = ch_mode & affected_bits;
				break;

			default:
				return -3; /* failed! -- unknown error */
			}
	}

	return 0; /* successful call */
}
