#pragma once
#include "gtsong.hpp"

namespace instrument_view {

    void draw();

    struct InstrumentCopyBuffer {
        int                                                   instr_num;
        gt::Instrument                                        instr;
        gt::Array2<uint8_t, gt::MAX_TABLES, gt::MAX_TABLELEN> ltable;
        gt::Array2<uint8_t, gt::MAX_TABLES, gt::MAX_TABLELEN> rtable;
        void copy();
        void paste() const;
    };


} // namespace instrument_view

