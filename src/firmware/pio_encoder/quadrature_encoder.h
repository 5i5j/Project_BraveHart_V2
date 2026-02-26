// src/firmware/pio_encoder/quadrature_encoder.h
#ifndef QUADRATURE_ENCODER_H
#define QUADRATURE_ENCODER_H

#include "hardware/pio.h"
#include "quadrature_encoder.pio.h" // 自动生成的头文件

class QuadratureEncoder {
public:
    QuadratureEncoder(PIO pio, uint pin_base) : pio_(pio), pin_base_(pin_base) {
        // 自动寻找可用的状态机
        sm_ = pio_claim_unused_sm(pio_, true);
        uint offset = pio_add_program(pio_, &quadrature_encoder_program);
        quadrature_encoder_program_init(pio_, sm_, offset, pin_base_);
    }

    // 从 PIO 寄存器读取当前计数值
    int32_t get_count() {
        // PIO 内部计数通常是递减或递增，这里取决于 .pio 的具体实现
        // 简单起见，我们直接读取 RX FIFO 中的数据
        if (pio_sm_is_rx_fifo_empty(pio_, sm_)) {
            return last_count_;
        }
        last_count_ = (int32_t)pio_sm_get(pio_, sm_);
        return last_count_;
    }

private:
    PIO pio_;
    uint sm_;
    uint pin_base_;
    int32_t last_count_ = 0;
};

#endif