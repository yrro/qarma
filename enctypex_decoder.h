/*
GS enctypeX servers list decoder/encoder 0.1.3b
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

This is the algorithm used by ANY new and old game which contacts the Gamespy master server.
It has been written for being used in gslist so there are no explanations or comments here,
if you want to understand something take a look to gslist.c

    Copyright 2008-2012 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

typedef struct {
    unsigned char   encxkey[261];   // static key
    int             offset;         // everything decrypted till now (total)
    int             start;          // where starts the buffer (so how much big is the header), this is the only one you need to zero
} enctypex_data_t;

int enctypex_decoder_rand_validate (unsigned char* validate);
unsigned char *enctypex_decoder(unsigned char* key, unsigned char* validate, unsigned char* data, int* datalen, enctypex_data_t* enctypex_data);
int enctypex_decoder_convert_to_ipport(unsigned char *data, int datalen, unsigned char *out, unsigned char *infobuff, int infobuff_size, int infobuff_offset);
