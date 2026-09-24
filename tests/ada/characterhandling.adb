with Ada.Characters.Handling; use Ada.Characters.Handling;
with Ada.Characters.Latin_1; use Ada.Characters.Latin_1;
with Ada.Text_IO;
procedure CharacterHandling is
    procedure check (condition : Boolean; message : String) is
    begin
        if not condition then
            Ada.Text_IO.Put_Line (message);
            raise Program_Error;
        end if;
    end check;
    text : String (8 .. 10) := "aZ!";
    empty : String (5 .. 4) := "";
    last : String (Integer'Last .. Integer'Last) := "a";
begin
    check (NUL = Character'Val (0) and LF = Character'Val (10), "controls");
    check (IS4 = FS and NBSP = No_Break_Space and Minus_Sign = '-', "aliases");
    check (UC_A_Grave = Character'Val (192) and LC_Y_Diaeresis = Character'Last, "Latin-1");
    for p in 0 .. 255 loop
        declare
            c : Character := Character'Val (p);
        begin
            check (Is_Control (c) /= Is_Graphic (c), "classification partition");
            check (Is_Alphanumeric (c) = (Is_Letter (c) or Is_Digit (c)), "alphanumeric");
            check (Is_Letter (c) = (Is_Lower (c) or Is_Upper (c)), "letters");
            check (Is_Decimal_Digit (c) = Is_Digit (c), "digit alias");
            check (Is_Special (c) = (Is_Graphic (c) and not Is_Alphanumeric (c)), "special");
            check (To_Lower (To_Lower (c)) = To_Lower (c), "lower idempotence");
            check (To_Upper (To_Upper (c)) = To_Upper (c), "upper idempotence");
            check (To_Basic (To_Basic (c)) = To_Basic (c), "basic idempotence");
            check (not Is_Mark (c), "no combining marks");
            check (Is_ISO_646 (c) = (p < 128), "ISO range");
        end;
    end loop;
    check (Is_Control (Character'Val (159)) and Is_Graphic (NBSP), "control boundary");
    check (not Is_Letter (Multiplication_Sign) and not Is_Letter (Division_Sign), "letter gaps");
    check (Is_Lower (LC_German_Sharp_S) and To_Upper (LC_German_Sharp_S) = LC_German_Sharp_S, "sharp s");
    check (To_Upper (LC_Y_Diaeresis) = LC_Y_Diaeresis, "unrepresentable uppercase");
    check (To_Lower (UC_Icelandic_Thorn) = LC_Icelandic_Thorn, "thorn");
    check (To_Upper (LC_A_Grave) = UC_A_Grave, "accented uppercase");
    check (To_Basic (UC_O_Oblique_Stroke) = 'O' and To_Basic (LC_Y_Diaeresis) = 'y', "basic conversion");
    check (Is_Basic (UC_AE_Diphthong) and not Is_Basic (UC_A_Grave), "basic classification");
    check (Is_Hexadecimal_Digit ('F') and Is_Hexadecimal_Digit ('9') and not Is_Hexadecimal_Digit ('G'), "hex");
    check (Is_Line_Terminator (NEL) and not Is_Line_Terminator (HT), "line terminators");
    check (Is_Other_Format (Soft_Hyphen) and Is_Punctuation_Connector ('_'), "format");
    check (Is_Space (NBSP) and not Is_Space (HT), "spaces");
    check (To_Upper (text) = "AZ!" and To_Lower (text) = "az!", "string conversion");
    check (To_Basic ("a" & UC_A_Grave) = "aA", "basic string");
    check (To_ISO_646 ("x" & NBSP, '?') = "x?" and To_ISO_646 (NBSP) = ' ', "substitution");
    check (Is_ISO_646 (text) and not Is_ISO_646 ("x" & NBSP), "ISO string");
    check (To_Upper (text)'First = 1 and To_ISO_646 (text)'First = 1, "rebasing");
    check (To_Lower (empty)'Length = 0 and To_Lower (empty)'First = 1, "empty result");
    check (Is_ISO_646 (empty) and To_Upper (last) = "A", "empty and maximum bounds");
    Ada.Text_IO.Put_Line ("characterhandling: passed");
end CharacterHandling;
