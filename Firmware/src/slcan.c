//
// slcan: Parse incoming and generate outgoing slcan messages
//

#include "stm32f0xx_hal.h"
#include <string.h>
#include "can.h"
#include "error.h"
#include "slcan.h"
#include "printf.h"
#include "usbd_cdc_if.h"

// Helper: convert single hex ASCII char to nibble (0..15), or -1 if invalid
static int8_t hexval(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}


// Parse an incoming CAN frame into an outgoing slcan message
int8_t slcan_parse_frame(uint8_t *buf, CAN_RxHeaderTypeDef *frame_header, uint8_t* frame_data)
{
    // Nouveau comportement : envoie une ligne par octet de donnée au format
    // >ID-Dn : valeur\r
    // Nous envoyons directement via CDC_Transmit_FS pour éviter d'augmenter
    // SLCAN_MTU ou de modifier la logique d'appelant.

    // Construire la chaîne d'identifiant
    uint8_t id_len = SLCAN_STD_ID_LEN;
    uint32_t can_id = frame_header->StdId;
    if (frame_header->IDE == CAN_ID_EXT)
    {
        id_len = SLCAN_EXT_ID_LEN;
        can_id = frame_header->ExtId;
    }

    char id_str[12];
    // Utiliser majuscules hexadécimales (comme avant)
    sprintf(id_str, "%0*X", id_len, (unsigned int)can_id);

    // Pour chaque octet de données, formater et transmettre une ligne
    for (uint8_t i = 0; i < frame_header->DLC; i++)
    {
        char line[64];
        // Par défaut on envoie la valeur en décimal. Si tu préfères hex,
        // remplace %u par 0x%02X ou modifie la chaîne ci-dessous.
        int n = snprintf(line, sizeof(line), ">%s-D%u : %u\r\n", id_str, (unsigned int)i, (unsigned int)frame_data[i]);
        if (n > 0)
        {
            CDC_Transmit_FS((uint8_t*)line, (uint16_t)n);
        }
    }

    // On retourne 0 pour indiquer à l'appelant que rien n'a à être retransmis
    // (les lignes ont été envoyées directement).
    return 0;
}


// Parse an incoming slcan command from the USB CDC port
int8_t slcan_parse_str(uint8_t *buf, uint8_t len)
{
	CAN_TxHeaderTypeDef frame_header;

	// Default to standard ID unless otherwise specified
	frame_header.IDE = CAN_ID_STD;
    frame_header.StdId = 0;
    frame_header.ExtId = 0;


    // Note: do NOT mutate the buffer globally here. We'll convert
    // ASCII hex/numeric characters locally where needed. This avoids
    // breaking commands like 'F' which need to inspect commas/X etc.

    // (local parsing will use file-scope hexval())


    // Process command
    switch(buf[0])
    {
		case 'O':
			// Open channel command
			can_enable();
			return 0;

		case 'C':
			// Close channel command
			can_disable();
			return 0;

		case 'S':
			// Set bitrate command

            // buf[1] is ASCII '0'..'9'
            if (buf[1] < '0' || buf[1] > '9') return -1;
            {
                uint8_t br = buf[1] - '0';
                if (br >= CAN_BITRATE_INVALID) return -1;
                can_set_bitrate(br);
            }
			return 0;

		case 'm':
		case 'M':
			// Set mode command
            // ASCII '1' enables
            if (buf[1] == '1') can_set_silent(1);
            else can_set_silent(0);
			return 0;

		case 'a':
		case 'A':
			// Set autoretry command
            if (buf[1] == '1') can_set_autoretransmit(1);
            else can_set_autoretransmit(0);
			return 0;

		case 'V':
		{
			// Report firmware version and remote
			char* fw_id = GIT_VERSION " " GIT_REMOTE "\r";
			CDC_Transmit_FS((uint8_t*)fw_id, strlen(fw_id));
			return 0;
		}

	    // Nonstandard!
		case 'E':
		{
	        // Report error register
			char errstr[64] = {0};
			snprintf_(errstr, 64, "CANable Error Register: %X", (unsigned int)error_reg());
			CDC_Transmit_FS((uint8_t*)errstr, strlen(errstr));
	        return 0;
		}

		case 'T':
	    	frame_header.IDE = CAN_ID_EXT;
		case 't':
			// Transmit data frame command
			frame_header.RTR = CAN_RTR_DATA;
			break;

		case 'R':
	    	frame_header.IDE = CAN_ID_EXT;
		case 'r':
			// Transmit remote frame command
			frame_header.RTR = CAN_RTR_REMOTE;
			break;
        case 'F': {
                    // Exemple : "F123,456,789\r" ou "FX\r"
                    if (buf[1] == 'X' || buf[1] == 'x') {
                        can_disable_filter();
                        char msg[] = "Filter OFF\r";
                        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
                        return 0;
                    }

                    // Quick ACK so the host knows the F command was received
                    CDC_Transmit_FS((uint8_t*)">Fcmd\r\n", 6);

                    uint32_t ids[MAX_FILTER_IDS];
                    uint8_t count = 0;
                    uint32_t current_id = 0;

                    for (uint8_t i = 1; i < len && count < MAX_FILTER_IDS; i++) {
                        uint8_t c = buf[i];

                        if (c == ',' || c == '\r') {
                            if (current_id > 0) {
                                ids[count++] = current_id;
                                current_id = 0;
                            }
                            if (c == '\r') break;
                        } else {
                            int8_t v = hexval(c);
                            if (v < 0) {
                                // invalid char -> abort this command
                                break;
                            }
                            current_id = (current_id << 4) | (uint32_t)v;
                        }
                    }

                    // The input buffer passed to slcan_parse_str does not include '\r',
                    // so flush any pending current_id after the loop.
                    if (current_id > 0 && count < MAX_FILTER_IDS) {
                        ids[count++] = current_id;
                        current_id = 0;
                    }

                    if (count > 0)
                        can_set_filters(ids, count);

                    // If CAN is not on bus, inform the host and return: no data will be available
                    if (!can_is_on_bus()) {
                        char msg_busy[64];
                        snprintf(msg_busy, sizeof(msg_busy), "CAN is OFF_BUS - send Sx and O to open\r\n");
                        CDC_Transmit_FS((uint8_t*)msg_busy, strlen(msg_busy));
                        return 0;
                    }

                    // For each ID requested, send exactly 8 lines with the last seen
                    // data for that ID. Format: >ID-Dn : value\r\n
                    for (uint8_t idx = 0; idx < count; idx++) {
                        uint8_t last_data[8] = {0};
                        uint8_t last_dlc = 0;
                        if (can_get_last_data(ids[idx], last_data, &last_dlc)) {
                            // use last_data for available bytes, other bytes default to 0
                        } else {
                            // no data seen yet; default bytes are 0
                        }

                        // Format ID string: determine whether this id is standard or extended
                        char id_str[12];
                        // If id fits in 11 bits (0x7FF) it's standard (3 hex chars), else extended (8)
                        if (ids[idx] <= 0x7FF) {
                            sprintf(id_str, "%03X", (unsigned int)ids[idx]);
                        } else {
                            sprintf(id_str, "%08X", (unsigned int)ids[idx]);
                        }

                        // Batch lines into TX_BUF_SIZE-sized packets to avoid
                        // overwhelming the USB layer with many small transmits.
                        char outbuf[TX_BUF_SIZE];
                        uint16_t outpos = 0;
                        for (uint8_t b = 0; b < 8; b++) {
                            char line[64];
                            uint8_t val = last_data[b];
                            int n = snprintf(line, sizeof(line), ">%s-D%u : %u\r\n", id_str, (unsigned int)b, (unsigned int)val);
                            if (n <= 0) continue;
                            // If line doesn't fit, flush current buffer first
                            if (outpos + n > TX_BUF_SIZE) {
                                CDC_Transmit_FS((uint8_t*)outbuf, outpos);
                                outpos = 0;
                            }
                            // copy line into outbuf
                            memcpy(&outbuf[outpos], line, n);
                            outpos += n;
                        }
                        // send remaining
                        if (outpos > 0) CDC_Transmit_FS((uint8_t*)outbuf, outpos);
                    }

                    return 0;
                }

    	default:
    		// Error, unknown command
    		return -1;
    }



    // Save CAN ID depending on ID type
    uint8_t msg_position = 1; // position in buf after command char
    if (frame_header.IDE == CAN_ID_EXT) {
        frame_header.ExtId = 0;
        for (uint8_t k = 0; k < SLCAN_EXT_ID_LEN; k++) {
            int8_t v = hexval(buf[msg_position++]);
            if (v < 0) return -1;
            frame_header.ExtId = (frame_header.ExtId << 4) | (uint32_t)v;
        }
    }
    else {
        frame_header.StdId = 0;
        for (uint8_t k = 0; k < SLCAN_STD_ID_LEN; k++) {
            int8_t v = hexval(buf[msg_position++]);
            if (v < 0) return -1;
            frame_header.StdId = (frame_header.StdId << 4) | (uint32_t)v;
        }
    }

    // Attempt to parse DLC and check sanity (ASCII digit)
    if (msg_position >= len) return -1;
    if (buf[msg_position] < '0' || buf[msg_position] > '8') return -1;
    frame_header.DLC = buf[msg_position++] - '0';

    // Copy frame data to buffer (each byte is two ASCII hex chars)
    uint8_t frame_data[8] = {0};
    for (uint8_t j = 0; j < frame_header.DLC; j++) {
        if (msg_position + 1 >= len) return -1;
        int8_t hi = hexval(buf[msg_position++]);
        int8_t lo = hexval(buf[msg_position++]);
        if (hi < 0 || lo < 0) return -1;
        frame_data[j] = (hi << 4) | lo;
    }

    // Transmit the message
    can_tx(&frame_header, frame_data);

    return 0;
}

