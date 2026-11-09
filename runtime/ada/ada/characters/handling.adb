package body Ada.Characters.Handling is
    function Is_Control (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return p <= 31 or (p >= 127 and p <= 159);
    end Is_Control;

    function Is_Graphic (Item : Character) return Boolean is
    begin
        return not Is_Control (Item);
    end Is_Graphic;

    function Is_Letter (Item : Character) return Boolean is
    begin
        return Is_Lower (Item) or Is_Upper (Item);
    end Is_Letter;

    function Is_Lower (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return (Item >= 'a' and Item <= 'z') or (p >= 223 and p <= 246) or p >= 248;
    end Is_Lower;

    function Is_Upper (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return (Item >= 'A' and Item <= 'Z') or (p >= 192 and p <= 214) or (p >= 216 and p <= 222);
    end Is_Upper;

    function Is_Basic (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return (Item >= 'A' and Item <= 'Z') or (Item >= 'a' and Item <= 'z')
            or p = 198 or p = 230 or p = 208 or p = 240
            or p = 222 or p = 254 or p = 223;
    end Is_Basic;

    function Is_Digit (Item : Character) return Boolean is
    begin
        return Item >= '0' and Item <= '9';
    end Is_Digit;

    function Is_Decimal_Digit (Item : Character) return Boolean is
    begin
        return Is_Digit (Item);
    end Is_Decimal_Digit;

    function Is_Hexadecimal_Digit (Item : Character) return Boolean is
    begin
        return Is_Digit (Item) or (Item >= 'A' and Item <= 'F') or (Item >= 'a' and Item <= 'f');
    end Is_Hexadecimal_Digit;

    function Is_Alphanumeric (Item : Character) return Boolean is
    begin
        return Is_Letter (Item) or Is_Digit (Item);
    end Is_Alphanumeric;

    function Is_Special (Item : Character) return Boolean is
    begin
        return Is_Graphic (Item) and not Is_Alphanumeric (Item);
    end Is_Special;

    function Is_Line_Terminator (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return (p >= 10 and p <= 13) or p = 133;
    end Is_Line_Terminator;

    function Is_Mark (Item : Character) return Boolean is
    begin
        return False;
    end Is_Mark;

    function Is_Other_Format (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return p = 173;
    end Is_Other_Format;

    function Is_Punctuation_Connector (Item : Character) return Boolean is
    begin
        return Item = '_';
    end Is_Punctuation_Connector;

    function Is_Space (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return p = 32 or p = 160;
    end Is_Space;

    function Is_ISO_646 (Item : Character) return Boolean is
        p : constant Integer := Character'Pos (Item);
    begin
        return p <= 127;
    end Is_ISO_646;

    function To_Lower (Item : Character) return Character is
    begin
        if Is_Upper (Item) then
            return Character'Val (Character'Pos (Item) + 32);
        end if;
        return Item;
    end To_Lower;

    function To_Upper (Item : Character) return Character is
    begin
        if Is_Lower (Item) and Item /= Character'Val (223) and Item /= Character'Val (255) then
            return Character'Val (Character'Pos (Item) - 32);
        end if;
        return Item;
    end To_Upper;

    function To_Basic (Item : Character) return Character is
        p : constant Integer := Character'Pos (Item);
    begin
        case p is
            when 192 .. 197 => return 'A';
            when 199 => return 'C';
            when 200 .. 203 => return 'E';
            when 204 .. 207 => return 'I';
            when 209 => return 'N';
            when 210 .. 214 | 216 => return 'O';
            when 217 .. 220 => return 'U';
            when 221 => return 'Y';
            when 224 .. 229 => return 'a';
            when 231 => return 'c';
            when 232 .. 235 => return 'e';
            when 236 .. 239 => return 'i';
            when 241 => return 'n';
            when 242 .. 246 | 248 => return 'o';
            when 249 .. 252 => return 'u';
            when 253 | 255 => return 'y';
            when others => return Item;
        end case;
    end To_Basic;

    function To_Lower (Item : String) return String is
        result : String (1 .. Item'Length);
    begin
        for i in result'Range loop
            result (i) := To_Lower (Item (Item'First + (i - 1)));
        end loop;
        return result;
    end To_Lower;

    function To_Upper (Item : String) return String is
        result : String (1 .. Item'Length);
    begin
        for i in result'Range loop
            result (i) := To_Upper (Item (Item'First + (i - 1)));
        end loop;
        return result;
    end To_Upper;

    function To_Basic (Item : String) return String is
        result : String (1 .. Item'Length);
    begin
        for i in result'Range loop
            result (i) := To_Basic (Item (Item'First + (i - 1)));
        end loop;
        return result;
    end To_Basic;

    function Is_ISO_646 (Item : String) return Boolean is
    begin
        for i in Item'Range loop
            if not Is_ISO_646 (Item (i)) then
                return False;
            end if;
        end loop;
        return True;
    end Is_ISO_646;

    function To_ISO_646 (Item : Character; Substitute : ISO_646 := ' ') return ISO_646 is
    begin
        if Is_ISO_646 (Item) then
            return Item;
        end if;
        return Substitute;
    end To_ISO_646;

    function To_ISO_646 (Item : String; Substitute : ISO_646 := ' ') return String is
        result : String (1 .. Item'Length);
    begin
        for i in result'Range loop
            result (i) := To_ISO_646 (Item (Item'First + (i - 1)), Substitute);
        end loop;
        return result;
    end To_ISO_646;
end Ada.Characters.Handling;
