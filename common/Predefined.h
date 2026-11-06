#ifndef ADA_PREDEFINED_H
#define ADA_PREDEFINED_H

/* The supported target model uses 32-bit Integer priorities and little-endian
   bit numbering. These declarations do not implement priority scheduling. */
enum
{
    kSystemPriorityFirst = 0,
    kSystemPriorityLast = 30,
    kSystemInterruptPriorityLast = 31,
    kSystemDefaultPriority = (kSystemPriorityFirst + kSystemPriorityLast) / 2,
    kSystemName = 0,
    kSystemLowOrderFirst = 1
};

/* RM J.5: shared by the builtin declarations and their addressable storage. */
#define ADA_ASCII_CONSTANTS(X) \
    X(NUL, nul, 0) \
    X(SOH, soh, 1) \
    X(STX, stx, 2) \
    X(ETX, etx, 3) \
    X(EOT, eot, 4) \
    X(ENQ, enq, 5) \
    X(ACK, ack, 6) \
    X(BEL, bel, 7) \
    X(BS, bs, 8) \
    X(HT, ht, 9) \
    X(LF, lf, 10) \
    X(VT, vt, 11) \
    X(FF, ff, 12) \
    X(CR, cr, 13) \
    X(SO, so, 14) \
    X(SI, si, 15) \
    X(DLE, dle, 16) \
    X(DC1, dc1, 17) \
    X(DC2, dc2, 18) \
    X(DC3, dc3, 19) \
    X(DC4, dc4, 20) \
    X(NAK, nak, 21) \
    X(SYN, syn, 22) \
    X(ETB, etb, 23) \
    X(CAN, can, 24) \
    X(EM, em, 25) \
    X(SUB, sub, 26) \
    X(ESC, esc, 27) \
    X(FS, fs, 28) \
    X(GS, gs, 29) \
    X(RS, rs, 30) \
    X(US, us, 31) \
    X(DEL, del, 127) \
    X(Exclam, exclam, 33) \
    X(Quotation, quotation, 34) \
    X(Sharp, sharp, 35) \
    X(Dollar, dollar, 36) \
    X(Percent, percent, 37) \
    X(Ampersand, ampersand, 38) \
    X(Colon, colon, 58) \
    X(Semicolon, semicolon, 59) \
    X(Query, query, 63) \
    X(At_Sign, at_sign, 64) \
    X(L_Bracket, l_bracket, 91) \
    X(Back_Slash, back_slash, 92) \
    X(R_Bracket, r_bracket, 93) \
    X(Circumflex, circumflex, 94) \
    X(Underline, underline, 95) \
    X(Grave, grave, 96) \
    X(L_Brace, l_brace, 123) \
    X(Bar, bar, 124) \
    X(R_Brace, r_brace, 125) \
    X(Tilde, tilde, 126) \
    X(LC_A, lc_a, 97) \
    X(LC_B, lc_b, 98) \
    X(LC_C, lc_c, 99) \
    X(LC_D, lc_d, 100) \
    X(LC_E, lc_e, 101) \
    X(LC_F, lc_f, 102) \
    X(LC_G, lc_g, 103) \
    X(LC_H, lc_h, 104) \
    X(LC_I, lc_i, 105) \
    X(LC_J, lc_j, 106) \
    X(LC_K, lc_k, 107) \
    X(LC_L, lc_l, 108) \
    X(LC_M, lc_m, 109) \
    X(LC_N, lc_n, 110) \
    X(LC_O, lc_o, 111) \
    X(LC_P, lc_p, 112) \
    X(LC_Q, lc_q, 113) \
    X(LC_R, lc_r, 114) \
    X(LC_S, lc_s, 115) \
    X(LC_T, lc_t, 116) \
    X(LC_U, lc_u, 117) \
    X(LC_V, lc_v, 118) \
    X(LC_W, lc_w, 119) \
    X(LC_X, lc_x, 120) \
    X(LC_Y, lc_y, 121) \
    X(LC_Z, lc_z, 122)

#endif
