#include <stdio.h>

#include "LogHelper.h"
#include "FilterParm.h"
#include "MTVideoTools.h"


#ifdef __MINGW32__
#undef fseeko
#define fseeko(x, y, z) fseeko64(x, y, z)
#undef ftello
#define ftello(x)       ftello64(x)
#elif defined(_WIN32)
#undef fseeko
#define fseeko(x, y, z) _fseeki64(x, y, z)
#undef ftello
#define ftello(x)       _ftelli64(x)
#endif

#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define BE_16(x) ((((uint8_t*)(x))[0] <<  8) | ((uint8_t*)(x))[1])

#define BE_32(x) (((uint32_t)(((uint8_t*)(x))[0]) << 24) |  \
                             (((uint8_t*)(x))[1]  << 16) |  \
                             (((uint8_t*)(x))[2]  <<  8) |  \
                              ((uint8_t*)(x))[3])

#define BE_64(x) (((uint64_t)(((uint8_t*)(x))[0]) << 56) |  \
                  ((uint64_t)(((uint8_t*)(x))[1]) << 48) |  \
                  ((uint64_t)(((uint8_t*)(x))[2]) << 40) |  \
                  ((uint64_t)(((uint8_t*)(x))[3]) << 32) |  \
                  ((uint64_t)(((uint8_t*)(x))[4]) << 24) |  \
                  ((uint64_t)(((uint8_t*)(x))[5]) << 16) |  \
                  ((uint64_t)(((uint8_t*)(x))[6]) <<  8) |  \
                  ((uint64_t)( (uint8_t*)(x))[7]))

#define BE_FOURCC(ch0, ch1, ch2, ch3)           \
    ( (uint32_t)(unsigned char)(ch3)        |   \
     ((uint32_t)(unsigned char)(ch2) <<  8) |   \
     ((uint32_t)(unsigned char)(ch1) << 16) |   \
     ((uint32_t)(unsigned char)(ch0) << 24) )

#define QT_ATOM BE_FOURCC
/* top level atoms */
#define FREE_ATOM QT_ATOM('f', 'r', 'e', 'e')
#define JUNK_ATOM QT_ATOM('j', 'u', 'n', 'k')
#define MDAT_ATOM QT_ATOM('m', 'd', 'a', 't')
#define MOOV_ATOM QT_ATOM('m', 'o', 'o', 'v')
#define PNOT_ATOM QT_ATOM('p', 'n', 'o', 't')
#define SKIP_ATOM QT_ATOM('s', 'k', 'i', 'p')
#define WIDE_ATOM QT_ATOM('w', 'i', 'd', 'e')
#define PICT_ATOM QT_ATOM('P', 'I', 'C', 'T')
#define FTYP_ATOM QT_ATOM('f', 't', 'y', 'p')
#define UUID_ATOM QT_ATOM('u', 'u', 'i', 'd')

#define CMOV_ATOM QT_ATOM('c', 'm', 'o', 'v')
#define STCO_ATOM QT_ATOM('s', 't', 'c', 'o')
#define CO64_ATOM QT_ATOM('c', 'o', '6', '4')

#define ATOM_PREAMBLE_SIZE    8
#define COPY_BUFFER_SIZE   33554432

MTVideoTools::MTVideoTools()
{

}

MTVideoTools::~MTVideoTools()
{

}

int MTVideoTools::qtFastStart(const char *infileName, const char *outfileName)
{
    unsigned char atom_bytes[ATOM_PREAMBLE_SIZE];
    FILE *infile  = NULL;
    FILE *outfile = NULL;
    uint32_t atom_type   = 0;
    uint64_t atom_size   = 0;
    uint64_t atom_offset = 0;
    int64_t last_offset;
    unsigned char *moov_atom = NULL;
    unsigned char *ftyp_atom = NULL;
    uint64_t moov_atom_size;
    uint64_t ftyp_atom_size = 0;
    uint64_t i, j;
    uint32_t offset_count;
    uint64_t current_offset;
    int64_t start_offset = 0;
    unsigned char *copy_buffer = NULL;
    int bytes_to_copy;
    int ret;

    if (infileName == NULL || outfileName == NULL) {
        LOGE("Parm err!");
        return AV_PARM_ERR;
    }

    infile = fopen(infileName, "rb");
    if (!infile) {
        LOGE("Open file %s error ", infileName);
        ret = AV_FILE_ERR;
        perror(infileName);
        goto error_out;
    }

    /* traverse through the atoms in the file to make sure that 'moov' is
     * at the end */
    while (!feof(infile)) {
        if (fread(atom_bytes, ATOM_PREAMBLE_SIZE, 1, infile) != 1) {
            break;
        }
        atom_size = BE_32(&atom_bytes[0]);
        atom_type = BE_32(&atom_bytes[4]);

        /* keep ftyp atom */
        if (atom_type == FTYP_ATOM) {
            ftyp_atom_size = atom_size;
            free(ftyp_atom);
            ftyp_atom = (unsigned char *)malloc(ftyp_atom_size);
            if (!ftyp_atom) {
                ret = AV_STREAM_ERR;
                LOGE("could not allocate ftyp atom");
                goto error_out;
            }
            if (fseeko(infile, -ATOM_PREAMBLE_SIZE, SEEK_CUR) ||
                fread(ftyp_atom, atom_size, 1, infile) != 1 ||
                (start_offset = ftello(infile)) < 0) {
                ret = AV_STREAM_ERR;
                goto error_out;
            }
        } else {
            /* 64-bit special case */
            if (atom_size == 1) {
                if (fread(atom_bytes, ATOM_PREAMBLE_SIZE, 1, infile) != 1) {
                    break;
                }
                atom_size = BE_64(&atom_bytes[0]);
                ret = fseeko(infile, atom_size - ATOM_PREAMBLE_SIZE * 2, SEEK_CUR);
            } else {
                ret = fseeko(infile, atom_size - ATOM_PREAMBLE_SIZE, SEEK_CUR);
            }
            if (ret) {
                //ret = AV_FILE_ERR;
                perror(infileName);
                goto error_out;
            }
        }

        if ((atom_type != FREE_ATOM) &&
            (atom_type != JUNK_ATOM) &&
            (atom_type != MDAT_ATOM) &&
            (atom_type != MOOV_ATOM) &&
            (atom_type != PNOT_ATOM) &&
            (atom_type != SKIP_ATOM) &&
            (atom_type != WIDE_ATOM) &&
            (atom_type != PICT_ATOM) &&
            (atom_type != UUID_ATOM) &&
            (atom_type != FTYP_ATOM)) {
            LOGI("encountered non-QT top-level atom (is this a QuickTime file?)\n");
            break;
        }
        atom_offset += atom_size;

        if (atom_size < 8)
            break;
    }

    if (atom_type != MOOV_ATOM) {
        LOGE("last atom in file was not a moov atom\n");
        free(ftyp_atom);
        fclose(infile);
        return AV_EXIT_NORMAL;
    }

    /* moov atom was, in fact, the last atom in the chunk; load the whole
     * moov atom */
    if (fseeko(infile, -atom_size, SEEK_END)) {
        ret = AV_FILE_ERR;
        perror(infileName);
        goto error_out;
    }
    last_offset    = ftello(infile);
    if (last_offset < 0) {
        ret = AV_FILE_ERR;
        perror(infileName);
        goto error_out;
    }
    moov_atom_size = atom_size;
    moov_atom      = (unsigned char *)malloc(moov_atom_size);
    if (!moov_atom) {
        ret = AV_MALLOC_ERR;
        LOGE("could not allocate  moov atom");
        goto error_out;
    }
    if (fread(moov_atom, atom_size, 1, infile) != 1) {
        ret = AV_FILE_ERR;
        perror(infileName);
        goto error_out;
    }

    /* this utility does not support compressed atoms yet, so disqualify
     * files with compressed QT atoms */
    if (BE_32(&moov_atom[12]) == CMOV_ATOM) {
        ret = AV_CONFIG_ERR;
        LOGE("this utility does not support compressed moov atoms yet\n");
        goto error_out;
    }

    /* close; will be re-opened later */
    fclose(infile);
    infile = NULL;

    /* crawl through the moov chunk in search of stco or co64 atoms */
    for (i = 4; i < moov_atom_size - 4; i++) {
        atom_type = BE_32(&moov_atom[i]);
        if (atom_type == STCO_ATOM) {
            LOGI(" patching stco atom...\n");
            atom_size = BE_32(&moov_atom[i - 4]);
            if (i + atom_size - 4 > moov_atom_size) {
                LOGE(" bad atom size\n");
                ret = AV_SET_ERR;
                goto error_out;
            }
            offset_count = BE_32(&moov_atom[i + 8]);
            if (i + 12 + offset_count * UINT64_C(4) > moov_atom_size) {
                ret = AV_SET_ERR;
                LOGE(" bad atom size/element count\n");
                goto error_out;
            }
            for (j = 0; j < offset_count; j++) {
                current_offset  = BE_32(&moov_atom[i + 12 + j * 4]);
                current_offset += moov_atom_size;
                moov_atom[i + 12 + j * 4 + 0] = (current_offset >> 24) & 0xFF;
                moov_atom[i + 12 + j * 4 + 1] = (current_offset >> 16) & 0xFF;
                moov_atom[i + 12 + j * 4 + 2] = (current_offset >>  8) & 0xFF;
                moov_atom[i + 12 + j * 4 + 3] = (current_offset >>  0) & 0xFF;
            }
            i += atom_size - 4;
        } else if (atom_type == CO64_ATOM) {
            LOGI(" patching co64 atom...\n");
            atom_size = BE_32(&moov_atom[i - 4]);
            if (i + atom_size - 4 > moov_atom_size) {
                LOGE(" bad atom size\n");
                ret = AV_SET_ERR;
                goto error_out;
            }
            offset_count = BE_32(&moov_atom[i + 8]);
            if (i + 12 + offset_count * UINT64_C(8) > moov_atom_size) {
                LOGE(" bad atom size/element count\n");
                ret = AV_SET_ERR;
                goto error_out;
            }
            for (j = 0; j < offset_count; j++) {
                current_offset  = BE_64(&moov_atom[i + 12 + j * 8]);
                current_offset += moov_atom_size;
                moov_atom[i + 12 + j * 8 + 0] = (current_offset >> 56) & 0xFF;
                moov_atom[i + 12 + j * 8 + 1] = (current_offset >> 48) & 0xFF;
                moov_atom[i + 12 + j * 8 + 2] = (current_offset >> 40) & 0xFF;
                moov_atom[i + 12 + j * 8 + 3] = (current_offset >> 32) & 0xFF;
                moov_atom[i + 12 + j * 8 + 4] = (current_offset >> 24) & 0xFF;
                moov_atom[i + 12 + j * 8 + 5] = (current_offset >> 16) & 0xFF;
                moov_atom[i + 12 + j * 8 + 6] = (current_offset >>  8) & 0xFF;
                moov_atom[i + 12 + j * 8 + 7] = (current_offset >>  0) & 0xFF;
            }
            i += atom_size - 4;
        }
    }

    /* re-open the input file and open the output file */
    infile = fopen(infileName, "rb");
    if (!infile) {
        perror(infileName);
        LOGE("[%d]Open file %s error ", __LINE__, infileName);
        ret = AV_FILE_ERR;
        goto error_out;
    }

    if (start_offset > 0) { /* seek after ftyp atom */
        if (fseeko(infile, start_offset, SEEK_SET)) {
            perror(infileName);
            LOGE("[%d]fseeko file %s error ", __LINE__, infileName);
            ret = AV_FILE_ERR;
            goto error_out;
        }

        last_offset -= start_offset;
    }

    outfile = fopen(outfileName, "wb");
    if (!outfile) {
        ret = AV_FILE_ERR;
        perror(outfileName);
        goto error_out;
    }

    /* dump the same ftyp atom */
    if (ftyp_atom_size > 0) {
        LOGI(" writing ftyp atom...\n");
        if (fwrite(ftyp_atom, ftyp_atom_size, 1, outfile) != 1) {
            perror(outfileName);
            ret = AV_FILE_ERR;
            goto error_out;
        }
    }

    /* dump the new moov atom */
    LOGI(" writing moov atom...\n");
    if (fwrite(moov_atom, moov_atom_size, 1, outfile) != 1) {
        perror(outfileName);
        ret = AV_FILE_ERR;
        goto error_out;
    }

    /* copy the remainder of the infile, from offset 0 -> last_offset - 1 */
    bytes_to_copy = MIN(COPY_BUFFER_SIZE, last_offset);
    copy_buffer = (unsigned char *)malloc(bytes_to_copy);
    if (!copy_buffer) {
        ret = AV_MALLOC_ERR;
        LOGE("could not allocate %d bytes for copy_buffer\n", bytes_to_copy);
        goto error_out;
    }
    LOGI(" copying rest of file...\n");
    while (last_offset) {
        bytes_to_copy = MIN(bytes_to_copy, last_offset);

        if (fread(copy_buffer, bytes_to_copy, 1, infile) != 1) {
            perror(infileName);
            ret = AV_FILE_ERR;
            goto error_out;
        }
        if (fwrite(copy_buffer, bytes_to_copy, 1, outfile) != 1) {
            perror(outfileName);
            ret = AV_FILE_ERR;
            goto error_out;
        }
        last_offset -= bytes_to_copy;
    }

    if (infile) {
        fclose(infile);
    }

    if (outfile) {
        fclose(outfile);
    }
    if (moov_atom) {
        free(moov_atom);
        moov_atom = NULL;
    }
    if (ftyp_atom) {
        free(ftyp_atom);
        ftyp_atom = NULL;
    }
    if (copy_buffer) {
        free(copy_buffer);
        copy_buffer = NULL;
    }

    return 0;

error_out:
    if (infile)
        fclose(infile);
    if (outfile)
        fclose(outfile);
    if (moov_atom) {
        free(moov_atom);
        moov_atom = NULL;
    }
    if (ftyp_atom) {
        free(ftyp_atom);
        ftyp_atom = NULL;
    }
    if (copy_buffer) {
        free(copy_buffer);
        copy_buffer = NULL;
    }
    return ret;

}