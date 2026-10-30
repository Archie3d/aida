-- RM A.4.6 Latin-1 sets and mappings; no body elaboration is required.
package Ada.Strings.Maps.Constants is
    pragma Pure;
    Control_Set : constant Character_Set;
    Graphic_Set : constant Character_Set;
    Letter_Set : constant Character_Set;
    Lower_Set : constant Character_Set;
    Upper_Set : constant Character_Set;
    Basic_Set : constant Character_Set;
    Decimal_Digit_Set : constant Character_Set;
    Hexadecimal_Digit_Set : constant Character_Set;
    Alphanumeric_Set : constant Character_Set;
    Special_Set : constant Character_Set;
    ISO_646_Set : constant Character_Set;
    Lower_Case_Map : constant Character_Mapping;
    Upper_Case_Map : constant Character_Mapping;
    Basic_Map : constant Character_Mapping;
private
    Control_Set : constant Character_Set :=
        (bits => (
            Character'Val (0) .. Character'Val (31) => True,
            Character'Val (127) .. Character'Val (159) => True,
            others => False));
    Graphic_Set : constant Character_Set :=
        (bits => (
            Character'Val (32) .. Character'Val (126) => True,
            Character'Val (160) .. Character'Val (255) => True,
            others => False));
    Letter_Set : constant Character_Set :=
        (bits => (
            Character'Val (65) .. Character'Val (90) => True,
            Character'Val (97) .. Character'Val (122) => True,
            Character'Val (192) .. Character'Val (214) => True,
            Character'Val (216) .. Character'Val (246) => True,
            Character'Val (248) .. Character'Val (255) => True,
            others => False));
    Lower_Set : constant Character_Set :=
        (bits => (
            Character'Val (97) .. Character'Val (122) => True,
            Character'Val (223) .. Character'Val (246) => True,
            Character'Val (248) .. Character'Val (255) => True,
            others => False));
    Upper_Set : constant Character_Set :=
        (bits => (
            Character'Val (65) .. Character'Val (90) => True,
            Character'Val (192) .. Character'Val (214) => True,
            Character'Val (216) .. Character'Val (222) => True,
            others => False));
    Basic_Set : constant Character_Set :=
        (bits => (
            Character'Val (65) .. Character'Val (90) => True,
            Character'Val (97) .. Character'Val (122) => True,
            Character'Val (198) => True,
            Character'Val (230) => True,
            Character'Val (208) => True,
            Character'Val (240) => True,
            Character'Val (222) .. Character'Val (223) => True,
            Character'Val (254) => True,
            others => False));
    Decimal_Digit_Set : constant Character_Set :=
        (bits => (
            Character'Val (48) .. Character'Val (57) => True,
            others => False));
    Hexadecimal_Digit_Set : constant Character_Set :=
        (bits => (
            Character'Val (48) .. Character'Val (57) => True,
            Character'Val (65) .. Character'Val (70) => True,
            Character'Val (97) .. Character'Val (102) => True,
            others => False));
    Alphanumeric_Set : constant Character_Set :=
        (bits => (
            Character'Val (48) .. Character'Val (57) => True,
            Character'Val (65) .. Character'Val (90) => True,
            Character'Val (97) .. Character'Val (122) => True,
            Character'Val (192) .. Character'Val (214) => True,
            Character'Val (216) .. Character'Val (246) => True,
            Character'Val (248) .. Character'Val (255) => True,
            others => False));
    Special_Set : constant Character_Set :=
        (bits => (
            Character'Val (32) .. Character'Val (47) => True,
            Character'Val (58) .. Character'Val (64) => True,
            Character'Val (91) .. Character'Val (96) => True,
            Character'Val (123) .. Character'Val (126) => True,
            Character'Val (160) .. Character'Val (191) => True,
            Character'Val (215) => True,
            Character'Val (247) => True,
            others => False));
    ISO_646_Set : constant Character_Set :=
        (bits => (
            Character'Val (0) .. Character'Val (127) => True,
            others => False));
    Lower_Case_Map : constant Character_Mapping :=
        (offsets => (
            Character'Val (65) .. Character'Val (90) => 32,
            Character'Val (192) .. Character'Val (214) => 32,
            Character'Val (216) .. Character'Val (222) => 32,
            others => 0));
    Upper_Case_Map : constant Character_Mapping :=
        (offsets => (
            Character'Val (97) .. Character'Val (122) => -32,
            Character'Val (224) .. Character'Val (246) => -32,
            Character'Val (248) .. Character'Val (254) => -32,
            others => 0));
    Basic_Map : constant Character_Mapping :=
        (offsets => (
            Character'Val (192) => -127,
            Character'Val (193) => -128,
            Character'Val (194) => -129,
            Character'Val (195) => -130,
            Character'Val (196) => -131,
            Character'Val (197) => -132,
            Character'Val (199) => -132,
            Character'Val (200) => -131,
            Character'Val (201) => -132,
            Character'Val (202) => -133,
            Character'Val (203) => -134,
            Character'Val (204) => -131,
            Character'Val (205) => -132,
            Character'Val (206) => -133,
            Character'Val (207) => -134,
            Character'Val (209) => -131,
            Character'Val (210) => -131,
            Character'Val (211) => -132,
            Character'Val (212) => -133,
            Character'Val (213) => -134,
            Character'Val (214) => -135,
            Character'Val (216) => -137,
            Character'Val (217) => -132,
            Character'Val (218) => -133,
            Character'Val (219) => -134,
            Character'Val (220) => -135,
            Character'Val (221) => -132,
            Character'Val (224) => -127,
            Character'Val (225) => -128,
            Character'Val (226) => -129,
            Character'Val (227) => -130,
            Character'Val (228) => -131,
            Character'Val (229) => -132,
            Character'Val (231) => -132,
            Character'Val (232) => -131,
            Character'Val (233) => -132,
            Character'Val (234) => -133,
            Character'Val (235) => -134,
            Character'Val (236) => -131,
            Character'Val (237) => -132,
            Character'Val (238) => -133,
            Character'Val (239) => -134,
            Character'Val (241) => -131,
            Character'Val (242) => -131,
            Character'Val (243) => -132,
            Character'Val (244) => -133,
            Character'Val (245) => -134,
            Character'Val (246) => -135,
            Character'Val (248) => -137,
            Character'Val (249) => -132,
            Character'Val (250) => -133,
            Character'Val (251) => -134,
            Character'Val (252) => -135,
            Character'Val (253) => -132,
            Character'Val (255) => -134,
            others => 0));
end Ada.Strings.Maps.Constants;
