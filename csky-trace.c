/*
 * CSKY trace
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/tracestub.h"
#include "exec/helper-proto.h"

extern bool is_gdbserver_start;
inline bool gen_mem_trace(void)
{
    if (tfilter.enable) {
        if (tfilter.event & TRACE_EVENT_DATA) {
            return true;
        }
    }
    return false;
}

inline bool gen_tb_trace(void)
{
    if (tfilter.enable) {
        if (tfilter.event & TRACE_EVENT_INSN) {
            return true;
        }
    }
    return false;
}

static bool stsp_range_match(uint32_t pc, uint32_t smask)
{
    struct trace_range *tr = NULL;
    int i;
    if (tfilter.stsp_num == 0) {
        if (smask == 0) {
            return true;
        } else {
            return false;
        }
    }
    for (i = 0; i < tfilter.stsp_num; i++) {
        tr = &tfilter.stsp_range[i];
        if (smask != 0) {
            if ((tr->start & smask ) == pc) {
                tr->start = pc;
                return true;
            } else if ((tr->end & smask) ==pc) {
                tr->end = pc;
                return true;
            } else {
                return false;
            }
        }
        if ((tr->start <= pc) && (tr->end  > pc)) {
            return true;
        }
    }
    return false;
}

static bool addr_range_match(uint32_t pc, uint32_t smask)
{
    struct trace_range *tr = NULL;
    int i;
    if (tfilter.addr_num == 0) {
        if (smask == 0) {
            return true;
        } else {
            return false;
        }
    }
    for (i = 0; i < tfilter.addr_num; i++) {
        tr = &tfilter.addr_range[i];
        if (smask != 0) {
            if ((tr->start & smask ) == pc) {
                tr->start = pc;
                return true;
            } else if ((tr->end & smask) == pc) {
                tr->end = pc;
                return true;
            } else {
                return false;
            }
        }
        if ((tr->start <= pc) && (tr->end > pc)) {
            return true;
        }
    }
    return false;
}

static bool data_range_match(uint32_t pc, uint32_t value)
{
    struct trace_data *td = NULL;
    struct trace_range *tr = NULL;
    int i;
    if (tfilter.data_num == 0) {
        return true;
    }
    for (i = 0; i < tfilter.data_num; i++) {
        td = &tfilter.data_range[i];
        tr = &td->data_addr;
        if ((tr->start <= pc) && (tr->end > pc)) {
            if (td->mode == TRACE_VALUE_SINGLE) {
                if (td->min == value) {
                    return true;
                }
            } else if (td->mode == TRACE_VALUE_RANGE) {
                if ((td->min <= value) && (td->max > value)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool trace_range_test(void *cpu, uint32_t pc, uint32_t smask)
{
    bool result = false;
    result = stsp_range_match(pc, smask);
    result |= addr_range_match(pc, smask);
    return result;
}
static int addr_trace_filter(CPUArchState *env, uint32_t pc)
{
    int result = 0;
    if (tfilter.enable) {
        if (tfilter.asid & TRACE_ASID_ENABLE_MASK) {
            if (ENV_GET_MMU(env)) {
                if (tfilter.asid != ENV_GET_ASID(env)) {
                    return result;
                }
            }
        }

        if (stsp_range_match(pc, 0)) {
            result |= STSP_RANGE_MATCH;
            if (addr_range_match(pc, 0)) {
                result |= ADDR_RANGE_MATCH;
            }
        }
    }
    return result;
}
static int data_trace_filter(CPUArchState *env,
    uint32_t pc, uint32_t addr, uint32_t value)
{
    int result = 0;
    if (tfilter.enable) {
        if (tfilter.asid & TRACE_ASID_ENABLE_MASK) {
            if (ENV_GET_MMU(env)) {
                if (tfilter.asid != ENV_GET_ASID(env)) {
                    return result;
                }
            }
        }
        if (stsp_range_match(pc, 0)) {
            result |= STSP_RANGE_MATCH;
            if (addr_range_match(addr, 0)) {
                result |= ADDR_RANGE_MATCH;
            }
            if (data_range_match(addr, value)) {
                    result |= DATA_RANGE_MATCH;
            }
        }
    }
    return result;
}

//#define CSKY_TRACE_DEBUG
#ifdef CSKY_TRACE_DEBUG
static int sync_trace_count;
#endif
void helper_csky_trace_icount(CPUArchState *env, uint32_t tb_pc, uint32_t icount)
{
    static long long total_csky_trace_icount;
    static long long last_csky_trace_icount;
    total_csky_trace_icount += icount;
    if ((total_csky_trace_icount - last_csky_trace_icount) > INSN_PER_PACKET) {
#ifdef CSKY_TRACE_DEBUG
        if (sync_trace_count % 100 == 0) {
            fprintf(stderr, ".");
            fflush(stderr);
        }
        sync_trace_count++;
#endif
        trace_send();
        last_csky_trace_icount = total_csky_trace_icount;
    }
    if (tfilter.enable) {
        if (tfilter.event & TRACE_EVENT_INSN) {
            int result = addr_trace_filter(env, tb_pc);
            if (result & STSP_RANGE_MATCH) {
                csky_trace_icount += icount;
            }
        }
    }
}

void helper_trace_tb_start(CPUArchState *env, uint32_t tb_pc)
{
    static int8_t lastbase;
    int8_t base = (tb_pc >> 24) & 0xff;
    int32_t offset = tb_pc & 0xffffff;

    int result = addr_trace_filter(env, tb_pc);
    if (result & STSP_RANGE_MATCH) {
        env->last_pc = tb_pc;
        if (base != lastbase) {
            write_trace_8(INST_BASE, 2 * sizeof(uint8_t), base);
            lastbase = base;
        }
        write_trace_8(INST_OFFSET, sizeof(uint32_t), offset);
        if (result & ADDR_RANGE_MATCH) {
            //write_trace_8(ADDR_CMPR_MATCH, sizeof(uint32_t), offset);
        }
    }
}

void helper_trace_tb_exit(uint32_t subtype, uint32_t offset)
{
    write_trace_8_8(INST_EXIT, sizeof(uint32_t), subtype, offset);
    if (is_gdbserver_start) {
        trace_send_immediately();
    }
}

static void helper_trace_ldst(CPUArchState *env, uint32_t pc,
    uint32_t rz, uint32_t addr, int type)
{
    int packlen = 0;
    packlen = 2 * sizeof(uint8_t) + sizeof(uint32_t);
    int result = data_trace_filter(env, pc, addr, rz);
    if (result & STSP_RANGE_MATCH) {
        switch (type) {
        case LD8U: case LD8S:
            write_trace_8_8(DATA_RADDR, packlen, sizeof(uint8_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            raddr_num++;
            break;
        case LD16U: case LD16S:
            write_trace_8_8(DATA_RADDR, packlen, sizeof(uint16_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            raddr_num++;
            break;
        case LD32U: case LD32S:
            write_trace_8_8(DATA_RADDR, packlen, sizeof(uint32_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            raddr_num++;
            break;
        case ST8:
            write_trace_8_8(DATA_WADDR, packlen, sizeof(uint8_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            waddr_num++;
            break;
        case ST16:
            write_trace_8_8(DATA_WADDR, packlen, sizeof(uint16_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            waddr_num++;
            break;
        case ST32:
            write_trace_8_8(DATA_WADDR, packlen, sizeof(uint32_t), addr);
            write_trace_8_8(DATA_VALUE, packlen, 0, rz);
            waddr_num++;
            break;
        default:
            break;
        }
        if (result & ADDR_RANGE_MATCH) {
        }
        if (result & DATA_RANGE_MATCH) {
        }
    }
}
void helper_trace_ld8u(CPUArchState *env, uint32_t pc,
                       uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD8U);
}

void helper_trace_ld16u(CPUArchState *env, uint32_t pc,
                        uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD16U);
}

void helper_trace_ld32u(CPUArchState *env, uint32_t pc,
                        uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD32U);
}

void helper_trace_ld8s(CPUArchState *env, uint32_t pc,
                       uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD8S);
}

void helper_trace_ld16s(CPUArchState *env, uint32_t pc,
                        uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD16S);
}

void helper_trace_ld32s(CPUArchState *env, uint32_t pc,
                        uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, LD32S);
}
void helper_trace_st8(CPUArchState *env, uint32_t pc,
                      uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, ST8);
}

void helper_trace_st16(CPUArchState *env, uint32_t pc,
                       uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, ST16);
}

void helper_trace_st32(CPUArchState *env, uint32_t pc,
                       uint32_t rz, uint32_t addr)
{
    helper_trace_ldst(env, pc, rz, addr, ST32);
}


void helper_trace_m_addr(CPUArchState *env, uint32_t pc,
                         uint32_t addr, uint32_t num,uint32_t type)
{
    int packlen = 0;
    packlen = 2 * sizeof(uint8_t) + sizeof(uint32_t);
    int result = addr_trace_filter(env, pc);
    if (result & STSP_RANGE_MATCH) {
        write_trace_8_8(type, packlen, num * sizeof(uint32_t), addr);
        if (type == 0x41) {
            waddr_num++;
        } else if (type == 0x40) {
            raddr_num++;
        }
        if (result & ADDR_RANGE_MATCH) {
        }
    }
}

void helper_trace_m_value(CPUArchState *env, uint32_t pc,
                          uint32_t addr, uint32_t value)
{
    int packlen = 0;
    packlen = 2 * sizeof(uint8_t) + sizeof(uint32_t);
    int result = data_trace_filter(env, pc, addr, value);
    if (result & STSP_RANGE_MATCH) {
        write_trace_8_8(DATA_VALUE, packlen, 0, value);
        if (result & ADDR_RANGE_MATCH) {
        }
        if (result & DATA_RANGE_MATCH) {
        }
    }
}


void helper_trace_update_tcr(CPUArchState *env)
{
    int mode = -1;
    int enable = 0;
    uint32_t *addr_index = &tfilter.addr_num;
    uint32_t *data_index = &tfilter.data_num;
    uint32_t *stsp_index = &tfilter.stsp_num;
    int value_index, value_mode, i;
    CPUState *cpu = ENV_GET_CPU(env);

    /* TRCEn enable */
    if ((env->cp14.tcr & 0x01) && (cpu->csky_trace_features & CSKY_TRACE)) {

        tfilter.enable = true;
        tfilter.event = env->cp14.ter; /* get all trace event */
        /* first send trace header */
        write_trace_header(env->cp14.ter);

        if (!(cpu->csky_trace_features & MEM_TRACE)) {
            tfilter.event &= ~(0x02);
        }
        if (!(cpu->csky_trace_features & TB_TRACE)) {
            tfilter.event &= ~(0x01);
        }
        tfilter.asid = env->cp14.asid;

        /* fill all range */
        for (i = 0; i < MAX_ADDR_CMPR_NUM - 1; i++) {
            mode = (env->cp14.addr_cmpr_config[i] & ADDR_CMPR_MODE_MASK)
                    >> ADDR_CMPR_MODE_START;
            enable = env->cp14.addr_cmpr_config[i] & ADDR_CMPR_ENABLE_MASK;
            if (enable) {
                switch (mode) {
                case INSN_ADDR_RANGE_CMPR:
                case DATA_ADDR_RANGE_CMPR: /* addr range match */
                    tfilter.addr_range[*addr_index].start
                                    = env->cp14.addr_cmpr[i];
                    tfilter.addr_range[*addr_index].end
                                    = env->cp14.addr_cmpr[++i];
                    (*addr_index)++;
                    break;
                case ASSOCIATE_VALUE_CMPR: /* associate value match */
                    value_index = env->cp14.addr_cmpr_config[i]
                                & ADDR_CMPR_DATA_MASK;
                    value_mode = env->cp14.data_cmpr_config[value_index]
                                & DATA_CMPR_MODE_MASK;
                    if (TRACE_VALUE_SINGLE == value_mode) {
                        tfilter.data_range[*data_index].mode
                                = value_mode;
                        tfilter.data_range[*data_index].min
                                = env->cp14.data_cmpr[value_index];
                    } else if (TRACE_VALUE_RANGE == value_mode) {
                        tfilter.data_range[*data_index].mode
                                = value_mode;
                        tfilter.data_range[*data_index].min
                                = env->cp14.data_cmpr[value_index];
                        tfilter.data_range[*data_index].max
                                = env->cp14.data_cmpr[value_index + 1];
                    } else {
                        return;
                    }
                    tfilter.data_range[*data_index].data_addr.start
                            =  env->cp14.addr_cmpr[i];
                    tfilter.data_range[*data_index].data_addr.end
                            =  env->cp14.addr_cmpr[++i];
                    (*data_index)++;
                    break;
                case STSP_RANGE_CMPR: /* stsp range match */
                    tfilter.stsp_range[*stsp_index].start
                            = env->cp14.addr_cmpr[i];
                    tfilter.stsp_range[*stsp_index].end
                            = env->cp14.addr_cmpr[++i];
                    (*stsp_index)++;
                    break;
                default:
                    break;
                }
            }
        }
    } else {
        memset(&tfilter, 0, sizeof(tfilter));
    }
}

