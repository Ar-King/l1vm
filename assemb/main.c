/*
 * This file main.c is part of L1vm.
 *
 * (c) Copyright Stefan Pietzonke (jay-t@gmx.net), 2017
 *
 * L1vm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * L1vm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with L1vm.  If not, see <http://www.gnu.org/licenses/>.
 */

//  l1asm RISC assembler
//

#include "../include/global.h"
#include "../include/opcodes.h"


S8 linenum ALIGN = 1;
U1 args[MAXARGS][MAXLINELEN];


S8 code_ind ALIGN = 0;
S8 data_ind ALIGN = 0;
S8 data_info_ind ALIGN = -1;
S8 code_max ALIGN = 4000000;
//S8 data_max ALIGN = 1048576;
S8 data_max ALIGN = 4000000;

// code
U1 code[4000000];

// data
U1 data[4000000];



U1 data_block = 0, code_block = 0;

U1 data_type;
S8 data_size ALIGN;

S2 arg_ind, arg_pos;

S8 label_ind ALIGN = -1;

// data lists
U1 data_list = 0;
S8 data_offset ALIGN = 0;
S8 data_offset_back ALIGN;
S8 data_save_offset ALIGN;

struct t_var t_var;
struct data_info data_info[MAXDATAINFO];
struct label label[MAXLABELS];

// protos
U1 checkdigit (U1 *str);
S8 get_temp_int (void);
F8 get_temp_double (void);
char *fgets_uni (char *str, int len, FILE *fptr);

void convtabs (U1 *str)
{
	S2 i, end;

	end = strlen ((const char *) str) - 1;

	for (i = 0; i <= end; i++)
	{
		if (str[i] == '\t')
		{
			str[i] = ' ';
		}
	}
}

S2 searchstr (U1 *str, U1 *srchstr, S2 start, S2 end, U1 case_sens)
{
	/* checked: ok! */

	S2 i, j = 0, pos = -1, str_len, srchstr_len;
	U1 ok = FALSE, check = TRUE;
	S2 new_end;
	U1 new_str, new_srchstr;

	str_len = strlen ((const char *) str);
	srchstr_len = strlen ((const char *) srchstr);

	if (start < 0 || start > str_len - 1)
	{
		i = 0;
	}
	else
	{
		i = start;
	}

	if (end == 0)
	{
		new_end = str_len - 1;
	}
	else
	{
		new_end = end;
	}

	while (! ok)
	{
		if (case_sens)
		{
			if (str[i] == srchstr[j])
			{
				pos = i;

				/* found start of searchstring, checking now */

				if (srchstr_len > 1)
				{
					for (j = j + 1; j <= srchstr_len - 1; j++)
					{
						if (i < new_end)
						{
							i++;
						}

						if (str[i] != srchstr[j]) check = FALSE;
					}
				}
				if (check)
				{
					ok = TRUE;
				}
				else
				{
					pos = -1;
				}
			}
			if (i < new_end)
			{
				i++;
			}
			else
			{
				ok = TRUE;
			}
		}
		else
		{
			new_str = str[i];
			new_srchstr = srchstr[j];

			if (str[i] >= 97 && str[i] <= 122)
			{
				new_str = str[i] - 32;
			}
			if (srchstr[j] >= 97 && srchstr[j] <= 122)
			{
				new_srchstr = srchstr[j] - 32;
			}

			if (new_str == new_srchstr)
			{
				pos = i;

				/* found start of searchstring, checking now */

				if (srchstr_len > 1)
				{
					for (j = j + 1; j <= srchstr_len - 1; j++)
					{
						if (i < new_end)
						{
							i++;
						}

						new_str = str[i];
						new_srchstr = srchstr[j];

						if (str[i] >= 97 && str[i] <= 122)
						{
							new_str = str[i] - 32;
						}
						if (srchstr[j] >= 97 && srchstr[j] <= 122)
						{
							new_srchstr = srchstr[j] - 32;
						}

						if (new_str != new_srchstr) check = FALSE;
					}
				}
				if (check)
				{
					ok = TRUE;
				}
				else
				{
					pos = -1;
				}
			}
			if (i < new_end)
			{
				i++;
			}
			else
			{
				ok = TRUE;
			}
		}
	}
	return (pos);
}

S2 get_args (U1 *line)
{
    S4 slen;
    S4 pos = 0, argstart, argend;

    U1 ok = 0;
    U1 arg = 0;
    slen = strlen ((const char *) line);

    arg_ind = -1;
	U1 string = 0;

    while (! ok)
    {
		printf ("get_args: line-pos: %i, char: %c\n", pos, line[pos]);
        if (line[pos] == ' ')
        {
            pos++;

            if (pos >= slen)
            {
                ok = 1;
            }
        }
        else
        {
            argstart = pos;
            arg_pos = 0;
            arg_ind++;
			arg = 0;
            while (! arg)
            {
				if (line[pos] == '"')
				{
					pos++;
					while (line[pos] != '"')
					{
						if (pos <= slen - 3)
						{
							if (line[pos] == '@' && line[pos + 1] == '@' && line[pos + 2] == 'q')
							{
								// found quote code, insert it into string
								args[arg_ind][arg_pos] = '"';
								arg_pos++;
								if (arg_pos >= MAXLINELEN)
								{
									printf ("error: line %lli: argument too long!\n", linenum);
									return (1);
								}
								pos = pos + 3;
							}

							if (line[pos] == '@' && line[pos + 1] == '@' && line[pos + 2] == 'c')
							{
								// found comma code, insert it into string
								args[arg_ind][arg_pos] = ',';
								arg_pos++;
								if (arg_pos >= MAXLINELEN)
								{
									printf ("error: line %lli: argument too long!\n", linenum);
									return (1);
								}
								pos = pos + 3;
							}
						}

						if (line[pos] != '"')
						{
							args[arg_ind][arg_pos] = line[pos];
							arg_pos++;
						}
						if (arg_pos >= MAXLINELEN)
						{
							printf ("error: line %lli: argument too long!\n", linenum);
							return (1);
						}
						pos++;

						if (pos >= slen)
						{
							ok = 1;
							break;
						}
					}
					args[arg_ind][arg_pos] = '\0';
					arg = 1;
					string = 1;
					if (line[pos + 1] == '\n') ok = 1;
					break;
				}
				else
				{
					if (line[pos] != ' ' && line[pos] != ',' && line[pos] != '\n')
					{
						args[arg_ind][arg_pos] = line[pos];
						arg_pos++;
						if (arg_pos >= MAXLINELEN)
						{
							printf ("error: line %lli: argument too long!\n", linenum);
							return (1);
						}
					}
					else
					{
						arg = 1;
					}
				}
				pos++;

				if (pos >= slen)
				{
					ok = 1;
					break;
				}
            }
            args[arg_ind][arg_pos] = '\0';
            printf ("[ get_args: ARGUMENTS %s ]\n", args[arg_ind]);

			// if (string == 1) arg_ind--;
            if (arg_ind >= MAXARGS)
            {
                printf ("error: line %lli too much arguments!\n", linenum);
                return (1);
            }
        }
    }
    return (0);
}


// write data ---------------------------------------------

S2 write_data_byte (S8 offset ALIGN, S8 val ALIGN, S8 size ALIGN)
{
	S8 i ALIGN;

	if (offset + size < data_max)
	{
		for (i = 0; i <= size; i++)
		{
        	data[offset] = (U1) val;
		}
        return (0);
    }
    else
    {
        return (1);
    }
}

S2 write_data_string (S8 offset ALIGN, U1 *str)
{
	S8 slen ALIGN;
	S8 i ALIGN;
	S8 j ALIGN;

	j = offset;
	slen = strlen ((const char *) str);
	if (offset + slen < data_max)
	{
		for (i = 0; i <= slen; i++)
		{
			data[j] = str[i];
			j++;
		}
		return (0);
	}
	else
	{
		return (1);
	}
}

S2 write_data_word (S8 offset ALIGN, S2 val, S8 size ALIGN)
{
	S8 i ALIGN;
    U1 *bptr = (U1 *) &val;

    if (offset + (sizeof (S2) * size) < data_max)
    {
		for (i = 1; i <= size; i++)
		{
#if MACHINE_BIG_ENDIAN
           data[offset] = *bptr;
           bptr++;
           data[offset + 1] = *bptr;
#else
           bptr = bptr + (sizeof (S2) - 1);
           data[offset] = *bptr;
           bptr--;
           data[offset + 1] = *bptr;
#endif
		}
        return (0);
    }
    else
    {
        return (1);
    }
}

S2 write_data_doubleword (S8 offset ALIGN, S4 val, S8 size ALIGN)
{
	S8 i ALIGN;
    U1 *bptr = (U1 *) &val;

    if (offset + (sizeof (S4) * size) < data_max)
    {
		for (i = 1; i <= size; i++)
		{

#if MACHINE_BIG_ENDIAN
        data[offset] = *bptr;
        bptr++;
        data[offset + 1] = *bptr;
        bptr++;
        data[offset + 2] = *bptr;
        bptr++;
        data[offset + 3] = *bptr;
#else
        bptr = bptr + (sizeof (S4) - 1);
        data[offset] = *bptr;
        bptr--;
        data[offset + 1] = *bptr;
        bptr--;
        data[offset + 2] = *bptr;
        bptr--;
        data[offset + 3] = *bptr;
#endif
		}
        return (0);
	}

    else
    {
        return (1);
    }
}

S2 write_data_quadword (S8 offset ALIGN, S8 val ALIGN, S8 size ALIGN)
{
    U1 *bptr = (U1 *) &val;
    S8 i ALIGN;

    if (offset + (sizeof (S8) * size) < data_max)
    {
		for (i = 1; i <= size; i++)
		{

#if MACHINE_BIG_ENDIAN
        data[offset] = *bptr;
        bptr++;
        data[offset + 1] = *bptr;
        bptr++;
        data[offset + 2] = *bptr;
        bptr++;
        data[offset + 3] = *bptr;
        bptr++;
        data[offset + 4] = *bptr;
        bptr++;
        data[offset + 5] = *bptr;
        bptr++;
        data[offset + 6] = *bptr;
        bptr++;
        data[offset + 7] = *bptr;

#else
        bptr = bptr + (sizeof (S8) - 1);
        data[offset] = *bptr;
        bptr--;
        data[offset + 1] = *bptr;
        bptr--;
        data[offset + 2] = *bptr;
        bptr--;
        data[offset + 3] = *bptr;
        bptr--;
        data[offset + 4] = *bptr;
        bptr--;
        data[offset + 5] = *bptr;
        bptr--;
        data[offset + 6] = *bptr;
        bptr--;
        data[offset + 7] = *bptr;
#endif
	}
        return (0);
    }
    else
    {
        return (1);
    }
}

S2 write_data_doublefloat (S8 offset ALIGN, F8 val ALIGN, S8 size ALIGN)
{
	S8 i ALIGN;
    U1 *bptr = (U1 *) &val;

    if (offset + (sizeof (F8) * size) < data_max)
    {

		for (i = 1; i <= size; i++)
		{
#if MACHINE_BIG_ENDIAN
        data[offset] = *bptr;
        bptr++;
        data[offset + 1] = *bptr;
        bptr++;
        data[offset + 2] = *bptr;
        bptr++;
        data[offset + 3] = *bptr;
        bptr++;
        data[offset + 4] = *bptr;
        bptr++;
        data[offset + 5] = *bptr;
        bptr++;
        data[offset + 6] = *bptr;
        bptr++;
        data[offset + 7] = *bptr;

#else
        bptr = bptr + (sizeof (F8) - 1);
        data[offset] = *bptr;
        bptr--;
        data[offset + 1] = *bptr;
        bptr--;
        data[offset + 2] = *bptr;
        bptr--;
        data[offset + 3] = *bptr;
        bptr--;
        data[offset + 4] = *bptr;
        bptr--;
        data[offset + 5] = *bptr;
        bptr--;
        data[offset + 6] = *bptr;
        bptr--;
        data[offset + 7] = *bptr;
#endif
		}
        return (0);
    }
    else
    {
        return (1);
    }
}

// write code ---------------------------------------------

S2 write_code_byte (S8 code_ind ALIGN, S8 val ALIGN)
{
    if (code_ind < code_max)
    {
        code[code_ind] = (U1) val;
        return (0);
    }
    else
    {
        return (1);
    }
}

S2 write_code_quadword (S8 code_ind ALIGN, S8 val ALIGN)
{
    U1 *bptr = (U1 *) &val;

    if (code_ind + sizeof (S8) < code_max)
    {
#if MACHINE_BIG_ENDIAN
        code[code_ind] = *bptr;
        bptr++;
        code[code_ind + 1] = *bptr;
        bptr++;
        code[code_ind + 2] = *bptr;
        bptr++;
        code[code_ind + 3] = *bptr;
        bptr++;
        code[code_ind + 4] = *bptr;
        bptr++;
        code[code_ind + 5] = *bptr;
        bptr++;
        code[code_ind + 6] = *bptr;
        bptr++;
        code[code_ind + 7] = *bptr;

#else
        bptr = bptr + (sizeof (S8) - 1);
        code[code_ind] = *bptr;
        bptr--;
        code[code_ind + 1] = *bptr;
        bptr--;
        code[code_ind + 2] = *bptr;
        bptr--;
        code[code_ind + 3] = *bptr;
        bptr--;
        code[code_ind + 4] = *bptr;
        bptr--;
        code[code_ind + 5] = *bptr;
        bptr--;
        code[code_ind + 6] = *bptr;
        bptr--;
        code[code_ind + 7] = *bptr;
#endif

        return (0);
    }
    else
    {
        return (1);
    }
}

S8 read_code_quadword (U1 *ptr)
{
	S8 num ALIGN = 0;
	U1 *bptr = (U1 *) &num;

#if MACHINE_BIG_ENDIAN
	*bptr = ptr[0];
	bptr++;
	*bptr = ptr[1];
	bptr++;
	*bptr = ptr[2];
	bptr++;
	*bptr = ptr[3];
	bptr++;
	*bptr = ptr[4];
	bptr++;
	*bptr = ptr[5];
	bptr++;
	*bptr = ptr[6];
	bptr++;
	*bptr = ptr[7];
#else
	*bptr = ptr[7];
	bptr++;
	*bptr = ptr[6];
	bptr++;
	*bptr = ptr[5];
	bptr++;
	*bptr = ptr[4];
	bptr++;
	*bptr = ptr[3];
	bptr++;
	*bptr = ptr[2];
	bptr++;
	*bptr = ptr[1];
	bptr++;
	*bptr = ptr[0];
#endif

	return (num);
}

S8 get_label_pos (U1 *labelname)
{
	S8 i ALIGN;
	S8 label_index ALIGN = -1;

	for (i = 0; i < MAXLABELS; i++)
	{
		if (strcmp ((const char *) label[i].name, (const char *) labelname) == 0)
		{
			label_index = i;
			break;
		}
	}
	return (label_index);
}

S2 write_code_labels (void)
{
	S8 i ALIGN;
	S8 j ALIGN;
	S8 label_index ALIGN;
	S8 label_pos ALIGN;
	S8 offset ALIGN;

	U1 err = 0;

	for (i = 16; i <= code_ind - 1; i = i + offset)
	{
		printf ("code: index %lli: opcode: %i\n", i, code[i]);

		if (code[i] == JMP|| code[i] == JMPI || code[i] == INCLSIJMPI || code[i] == DECGRIJMPI || code[i] == LOADL || code[i] == JSR)
		{
			if (code[i] == JMP)
			{
				label_index = read_code_quadword (&code[i + 1]);
				if (label[label_index].pos == -1)
				{
					printf ("ERROR: write_code_labels: label: %s not defined!\n", label[label_index].name);
					err = 1;
				}
				write_code_quadword(i + 1, label[label_index].pos);
			}

			if (code[i] == JMPI)
			{
				label_index = read_code_quadword (&code[i + 2]);
				if (label[label_index].pos == -1)
				{
					printf ("ERROR: write_code_labels: label: %s not defined!\n", label[label_index].name);
					err = 1;
				}
				write_code_quadword(i + 2, label[label_index].pos);
			}

			if (code[i] == INCLSIJMPI || code[i] == DECGRIJMPI)
			{
				label_index = read_code_quadword (&code[i + 3]);

				printf ("write_code_labels: label: inclsijmpi: %lli\n", label[label_index].pos);
				if (label[label_index].pos == -1)
				{
					printf ("ERROR: write_code_labels: label: %s not defined!\n", label[label_index].name);
					err = 1;
				}
				write_code_quadword(i + 3, label[label_index].pos);
			}

			if (code[i] == LOADL)
			{
				label_index = read_code_quadword (&code[i + 1]);
				if (label[label_index].pos == -1)
				{
					printf ("ERROR: write_code_labels: label: %s not defined!\n", label[label_index].name);
					err = 1;
				}
				write_code_quadword(i + 1, label[label_index].pos);
			}

			if (code[i] == JSR)
			{
				label_index = read_code_quadword (&code[i + 1]);
				if (label[label_index].pos == -1)
				{
					printf ("ERROR: write_code_labels: label: %s not defined!\n", label[label_index].name);
					err = 1;
				}
				write_code_quadword(i + 1, label[label_index].pos);
			}
		}

		offset = 0;

		for (j = 0; j < opcode[code[i]].args; j++)
		{
			switch (opcode[code[i]].type[j])
			{
				case I_REG:
				case D_REG:
					offset++;
					break;

				case DATA:
				case DATA_OFFS:
				case LABEL:
					offset = offset + sizeof (S8);
					break;

				case ALL:
					offset++;
					break;
			}
		}
		offset++;
		printf ("OFFSET: %lli\n", offset);
	}
	return (err);
}

S2 parse_line (U1 *line, S2 start, S2 end)
{
    S4 pos = start;
    S8 offset ALIGN;
    S8 datai ALIGN;
    F8 datad ALIGN;

	S8 label_pos ALIGN;
    S8 slen, i, j, d;

	S8 type_size;

	U1 opcode_found, data_found;

    if (line[0] == '.')
    {
        if (line[1] == 'd' && line[2] == 'a' && line[3] == 't' && line[4] == 'a')
        {
            // data block begins
            data_block = 1;
			printf ("line: %lli: data block.\n", linenum);
            return (0);
        }

        if (line[1] == 'c' && line[2] == 'o' && line[3] == 'd' && line[4] == 'e')
        {
            // code block begins
            code_block = 1;
			printf ("line: %lli: code block.\n", linenum);
            return (0);
        }

        if (line[1] == 'd' && line[2] == 'e' && line[3] == 'n' && line[4] == 'd')
        {
            // data block ends
            data_block = 2;
            return (0);
        }

        if (line[1] == 'c' && line[2] == 'e' && line[3] == 'n' && line[4] == 'd')
        {
            // code block ends
            code_block = 2;
            return (0);
        }
    }

        if (data_block == 1)
        {
            get_args (line);
            if (arg_ind < 2)
            {
                printf ("error: line %lli: too few arguments!\n", linenum);
                return (1);
            }

			printf ("ARG IND: %i\n", arg_ind);

			if (arg_ind >= 3)
			{
				data_list = 1;
			}

            if (args[0][0] == '@')
            {
				if (data_list == 0)
				{
                	if (checkdigit (args[1]) != 1)
                	{
                    	printf ("error: line %lli: offset not a number!\n", linenum);
                    	return (1);
                	}

                	offset = get_temp_int ();
                	offset += 8;	// data size = 8 bytes

					printf ("ARGS 2: %s\n", args[2]);

                	if (checkdigit (args[2]) == 1)
                	{
                    	if (t_var.digitstr_type == DOUBLEFLOAT)
                    	{
                        	datad = get_temp_double ();
                        	if (write_data_doublefloat (offset, datad, data_info[data_info_ind].size) != 0)
							{
								printf ("error: line %lli: out of data memory!\n", linenum);
							}
							// printf ("line %lli: doublefloat type\n", linenum);
                    	}
                    	else
                    	{
                        	datai = get_temp_int ();

							switch (data_info[data_info_ind].type)
                        	{
                            	case BYTE:
                                	if (write_data_byte (offset, (U1) datai, data_info[data_info_ind].size) != 0)
									{
										printf ("error: line %lli: out of data memory!\n", linenum);
									}
                                	break;

                            	case WORD:
                                	if (write_data_word (offset, (S2) datai, data_info[data_info_ind].size) != 0)
									{
										printf ("error: line %lli: out of data memory!\n", linenum);
									}
                                	break;

                            	case DOUBLEWORD:
                                	if (write_data_doubleword (offset, (S4) datai, data_info[data_info_ind].size) != 0)
									{
										printf ("error: line %lli: out of data memory!\n", linenum);
									}
                                	break;

                            	case QUADWORD:
                                	if (write_data_quadword (offset, datai, data_info[data_info_ind].size) != 0)
									{
										printf ("error: line %lli: out of data memory!\n", linenum);
									}
                                	break;
                        	}
                    	}
                    	data_info[data_info_ind].offset = offset;
                    	data_ind = data_info[data_info_ind].end;
                	}
                	else
                	{
                    	printf ("line: %lli DATA STRING: '%s'\n", linenum, args[2]);
                    	if (write_data_string (offset, args[2]) != 0)
						{
							printf ("error: line %lli: out of data memory!\n", linenum);
						}
						data_info[data_info_ind].offset = offset;
						data_ind = data_info[data_info_ind].end;
                	}
				}
				else
				{
					// data list

					if (data_offset == 0)
					{
						if (checkdigit (args[1]) != 1)
                		{
                    		printf ("error: line %lli: offset not a number!\n", linenum);
                    		return (1);
                		}

                		offset = get_temp_int ();
                		offset += 8;	// data size = 8 bytes

						data_offset = offset;
						data_save_offset = offset;
						j = 2;
					}
					else
					{
						j = 1;
						data_offset = data_offset_back;
						offset = data_save_offset;
					}

					printf ("data list arg ind: %i\n", arg_ind);

					for (i = j; i <= arg_ind; i++)
					{
						if (args[i][0] == ';')
						{
							// end of list mark, break, clear data list flag
							data_list = 0;
							data_info[data_info_ind].offset = offset;
							data_ind = data_info[data_info_ind].end;
						//	offset = data_offset;
							data_save_offset = data_offset;
							data_offset = 0;
							break;
						}

						if (checkdigit (args[i]) == 1)
	                	{
	                    	if (t_var.digitstr_type == DOUBLEFLOAT)
	                    	{
	                        	datad = get_temp_double ();
	                        	if (write_data_doublefloat (data_offset, datad, 1) != 0)
								{
									printf ("error: line %lli: out of data memory!\n", linenum);
								}
								printf ("line %lli: doublefloat type\n", linenum);
								data_offset = data_offset + sizeof (F8);
	                    	}
	                    	else
	                    	{
	                        	datai = get_temp_int ();

								switch (data_info[data_info_ind].type)
	                        	{
	                            	case BYTE:
	                                	if (write_data_byte (data_offset, (U1) datai, 1) != 0)
										{
											printf ("error: line %lli: out of data memory!\n", linenum);
										}
										data_offset = data_offset + 1;
	                                	break;

	                            	case WORD:
	                                	if (write_data_word (data_offset, (S2) datai, 1) != 0)
										{
											printf ("error: line %lli: out of data memory!\n", linenum);
										}
										data_offset = data_offset + sizeof (S2);
	                                	break;

	                            	case DOUBLEWORD:
	                                	if (write_data_doubleword (data_offset, (S4) datai, 1) != 0)
										{
											printf ("error: line %lli: out of data memory!\n", linenum);
										}
										data_offset = data_offset + sizeof (S4);
	                                	break;

	                            	case QUADWORD:
	                                	if (write_data_quadword (data_offset, datai, 1) != 0)
										{
											printf ("error: line %lli: out of data memory!\n", linenum);
										}
										data_offset = data_offset + sizeof (S8);
	                                	break;
	                        	}
	                    	}
	                	}
					}
					data_offset_back = data_offset;
					data_save_offset = offset;
					return (0);
            	}
			}
            else
            {
                // check data type
                if (args[0][0] == 'B')
                {
                    // byte data
                    data_type = BYTE;
                    data_size = 1;
                }

                if (args[0][0] == 'W')
                {
                    // byte data
                    data_type = WORD;
                    data_size = sizeof (S2);
                }

                if (args[0][0] == 'D')
                {
                    // byte data
                    data_type = DOUBLEWORD;
                    data_size = sizeof (S4);
                }

                if (args[0][0] == 'Q')
                {
                    // byte data
                    data_type = QUADWORD;
                    data_size = sizeof (S8);
                }

                if (args[0][0] == 'F')
                {
                    // byte data
                    data_type = DOUBLEFLOAT;
                    data_size = sizeof (F8);
                }

                if (data_type == UNKNOWN)
                {
                    printf ("error: line %lli: unknown data type!", linenum);
                    return (1);
                }

                // check data size
                if (checkdigit (args[1]) != 1)
                {
                    printf ("error: line %lli: size not a number!\n", linenum);
                    return (1);
                }

				if (data_info_ind < MAXDATAINFO)
				{
					data_info_ind++;
				}
				else
				{
					printf ("error: line %lli: data info overflow!\n", linenum);
					return (1);
				}

                data_info[data_info_ind].size = get_temp_int ();
				data_info[data_info_ind].size *= data_size;
                if (data_ind + data_info[data_info_ind].size > data_max)
                {
                    printf ("error: line %lli: size to big for max data!\n", linenum);
                    return (1);
                }
                data_info[data_info_ind].offset = data_ind;
                data_info[data_info_ind].end = data_ind + (data_size * data_info[data_info_ind].size);

                // data name
                strcpy ((char *) data_info[data_info_ind].name, (const char *) args[2]);
				data_info[data_info_ind].type_size = data_size;
				data_info[data_info_ind].type = data_type;
            }

        }

        if (code_block == 1)
        {
            get_args (line);

            if (args[0][0] == ':')
            {
				printf ("line %lli: label: %s\n", linenum, args[0]);
				label_pos = get_label_pos ((U1 *) args[0]);
				if (label_pos == -1)
				{
					// label name
					slen = strlen ((const char *) args[0]);
					if (slen < LABELLEN - 1)
					{
						if (label_ind < MAXLABELS)
						{
							label_ind++;
							strcpy ((char *) label[label_ind].name, (const char *) args[0]);
							label[label_ind].pos = code_ind;
						}
						else
						{
							printf ("error: line %lli: label list full!\n", linenum);
							return (1);
						}
					}
					else
					{
						printf ("error: line %lli: label name too long!\n", linenum);
						return (1);
					}
				}
				else
				{
					// set position to label set by jump opcode
					label[label_pos].pos = code_ind;
					printf ("label position set\n");
				}
            }
            else
            {
                // opcode
                opcode_found = 0;
                for (i = 0; i < MAXOPCODES; i++)
                {
                    if (strcmp ((const char *) args[0], (const char *) opcode[i].op) == 0)
                    {
                        opcode_found = 1;

                        if (code_ind >= code_max)
                        {
                            printf ("error: line %lli: code memory overflow!\n", linenum);
                            return (1);
                        }

                        if (arg_ind != opcode[i].args)
                        {
							printf ("error: line %lli: arguments mismatch!\n", linenum);
							return (1);
                        }

                        write_code_byte (code_ind, i);
                        code_ind++;

                        for (j = 1; j <= arg_ind; j++)
                        {
                            switch (opcode[i].type[j - 1])
                            {
                                case I_REG:
                                case D_REG:
                                    if (checkdigit (args[j]) == 1)
                                    {
                                        datai = get_temp_int ();
                                        write_code_byte (code_ind, datai);
										code_ind++;
                                    }
                                    else
                                    {
                                        printf ("error: line %lli: argument %lli not a number!\n", linenum, j);
                                        return (1);
                                    }
                                    break;

                                case DATA:
									data_found = 0;
                                    for (d = 0; d <= data_info_ind; d++)
                                    {
                                        if (strcmp ((const char *) data_info[d].name, (const char *) args[j]) == 0)
                                        {
                                            // found data entry

                                            write_code_quadword (code_ind, data_info[d].offset - 8);
											printf ("DATA: %s, offset: %lli\n", data_info[d].name, data_info[d].offset - 8);
											code_ind += sizeof (S8);
											data_found = 1;
                                            break;
                                        }
                                    }

                                    if (data_found == 0)
									{
										printf ("error: line %lli: data name not defined: '%s'!\n", linenum, args[j]);
										return (1);
									}
                                    break;

                                case DATA_OFFS:
                                    if (checkdigit (args[j]) == 1)
                                    {
                                        datai = get_temp_int ();
                                        write_code_quadword (code_ind, datai);
										code_ind += sizeof (S8);
                                    }
                                    else
                                    {
                                        printf ("error: line %lli: argument %lli not a number!\n", linenum, j);
                                        return (1);
                                    }
                                    break;

                                case LABEL:
                                    label_pos = get_label_pos ((U1 *) args[j]);
									if (label_pos == -1)
									{
										// label not set, set label in list

										slen = strlen ((const char *) args[j]);
										if (slen < LABELLEN - 1)
										{
											if (label_ind < MAXLABELS)
											{
												label_ind++;
												strcpy ((char *) label[label_ind].name, (const char *) args[j]);
												label[label_ind].pos = -1;	// position of label now undefined
												write_code_quadword (code_ind, label_ind);
												code_ind += sizeof (S8);
											}
											else
											{
												printf ("error: line %lli: label list full!\n", linenum);
												return (1);
											}
										}
										else
										{
											printf ("error: line %lli: label name too long!\n", linenum);
											return (1);
										}
									}
									else
									{
										write_code_quadword (code_ind, label_pos);
										code_ind += sizeof (S8);
									}
                                    break;

                                case ALL:
                                    if (checkdigit (args[j]) == 1)
                                    {
                                        datai = get_temp_int ();
                                        write_code_byte (code_ind, datai);
										code_ind += sizeof (U1);
                                    }
                                    else
                                    {
                                        printf ("error: line %lli: argument %lli not a number!\n", linenum, j);
                                        return (1);
                                    }
                                    break;
                            }
                        }
                        break;
                    }
                }

                if (opcode_found == 0)
                {
                    printf ("error: line %lli: unknown opcode!\n", linenum);
                    return (1);
                }
            }
        }
    return (0);
}

S2 parse (U1 *name)
{
    FILE *fptr;
    U1 asmname[512];
    S4 slen, pos;
    U1 rbuf[MAXLINELEN + 1];                        /* read-buffer for one line */
    char *read;

    slen = strlen ((const char *) name);
    U1 ok, err = 0;

    if (slen > 506)
    {
        printf ("ERROR: filename too long!\n");
        return (1);
    }

    strcpy ((char *) asmname, (const char *) name);
    strcat ((char *) asmname, ".l1asm");

    fptr = fopen ((const char *) asmname, "r");
    if (fptr == NULL)
    {
        printf ("ERROR: can't open assembly file '%s'!\n", asmname);
        return (1);
    }

    ok = TRUE;
    while (ok)
    {
        read = fgets_uni ((char *) rbuf, MAXLINELEN, fptr);

        if (read != NULL && (code_ind < code_max - 1))
        {
            convtabs (rbuf);                    /* convert the funny tabs into spaces! */
            slen = strlen ((const char *) rbuf);

			printf ("[ %s ]\n", rbuf);

            pos = searchstr (rbuf, (U1 *) REM_SB, 0, 0, TRUE);
            if (pos != -1)
            {
                // found comment

               // parse_line (rbuf, 0, pos - 1);
            }
            else
            {
                if (parse_line (rbuf, 0, slen - 1) != 0)
				{
					err = 1;
				}
            }
            linenum++;
        }
        else
		{
			ok = FALSE;
		}
    }
    fclose (fptr);
    return (err);
}

 S8 conv_quadword (S8 val ALIGN)
 {
	 S8 ret ALIGN;

	 U1 *valptr = (U1 *) &val;
	 U1 *retptr = (U1 *) &ret;

	 #if MACHINE_BIG_ENDIAN
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 retptr++; valptr++;
	 *retptr = *valptr;
	 #else
	 *retptr = valptr[7];
	 retptr++;
	 *retptr = valptr[6];
	 retptr++;
	 *retptr = valptr[5];
	 retptr++;
	 *retptr = valptr[4];
	 retptr++;
	 *retptr = valptr[3];
	 retptr++;
	 *retptr = valptr[2];
	 retptr++;
	 *retptr = valptr[1];
	 retptr++;
	 *retptr = valptr[0];
	 #endif

	 return (ret);
 }


S2 dump_object (U1 *name)
{
	FILE *fptr;
	U1 objname[512];
	S8 data_size ALIGN;
	S8 writesize ALIGN;
	S4 slen;

	S8 header ALIGN;
	S8 i ALIGN;
	S8 d ALIGN;

	slen = strlen ((const char *) name);
	U1 ok;

	if (slen > 506)
	{
		printf ("ERROR: filename too long!\n");
		return (1);
	}

	strcpy ((char *) objname, (const char *) name);
	strcat ((char *) objname, ".l1obj");

	fptr = fopen ((const char *) objname, "w");
	if (fptr == NULL)
	{
		printf ("ERROR: can't open object file '%s'!\n", objname);
		return (1);
	}

	// write object file header
	header = 0xBADBABEAABBCCDDE;
	write_code_quadword (0, header);

	// write code & data sizes to offset 0
	write_code_quadword (8, code_ind);


	data_size = data_ind;
	data_ind = 0;
	write_data_quadword (0, data_size, 1);


	writesize = fwrite ((U1 *) &code, sizeof (U1), code_ind, fptr);
	if (writesize != code_ind)
	{
		printf ("ERROR: can't write code section!\n");
		return (1);
	}

	printf ("object: codesize: %lli\n", code_ind);

	// write data info block

	if (fputc ('i', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('n', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('f', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('o', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	for (i = 0; i <= data_info_ind; i++)
	{
		writesize = fwrite ((U1 *) &data_info[i].type, sizeof (U1), 1, fptr);
		if (writesize != 1)
		{
			printf ("ERROR: can't write data info section type!\n");
			return (1);
		}
		d = conv_quadword (data_info[i].size);
		writesize = fwrite ((U1 *) &d, sizeof (S8), 1, fptr);
		if (writesize != 1)
		{
			printf ("ERROR: can't write data info section size!\n");
			return (1);
		}
	}

	if (fputc ('d', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('a', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('t', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	if (fputc ('a', fptr) == EOF)
	{
		printf ("ERROR: can't write data section header!\n");
		return (1);
	}

	writesize = fwrite ((U1 *) &data, sizeof (U1), data_size, fptr);
	if (writesize != data_size)
	{
		printf ("ERROR: can't write data section!\n");
		return (1);
	}

	printf ("object: datasize: %lli\n", data_size);

	fclose (fptr);
	return (0);
}

int main (int ac, char *av[])
{
    printf ("l1asm <asm-file>\n");
	printf ("(C) 2017-2018 Stefan Pietzonke\n");

    if (ac < 2)
    {
        exit (1);
    }

    //  space for header
    write_code_quadword (0, 0);
	code_ind += sizeof (S8);

    //  space for codesize
    write_code_quadword (0, 0);
	code_ind += sizeof (S8);

	// space for datasize
	write_data_quadword (0, 0, 1);
    data_ind += sizeof (S8);


    if (parse ((U1 *) av[1]) == 1)
	{
		printf ("ERRORS! can't write object file!\n");
		exit (1);
	}
	if (write_code_labels () == 0)
	{
		dump_object ((U1 *) av[1]);
	}
	else
	{
		printf ("ERRORS! can't write object file!\n");
		exit (1);
	}
	exit (0);
}
