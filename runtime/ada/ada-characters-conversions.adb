package body Ada.Characters.Conversions is
    function Is_Character (Item : Wide_Character) return Boolean is
    begin
        return Wide_Character'Pos (Item) <= 255;
    end Is_Character;
    function Is_String (Item : Wide_String) return Boolean is
    begin
        for I in Item'Range loop
            if not Is_Character (Item (I)) then return False; end if;
        end loop;
        return True;
    end Is_String;
    function Is_Character (Item : Wide_Wide_Character) return Boolean is
    begin
        return Wide_Wide_Character'Pos (Item) <= 255;
    end Is_Character;
    function Is_String (Item : Wide_Wide_String) return Boolean is
    begin
        for I in Item'Range loop
            if not Is_Character (Item (I)) then return False; end if;
        end loop;
        return True;
    end Is_String;
    function Is_Wide_Character (Item : Wide_Wide_Character) return Boolean is
    begin
        return Wide_Wide_Character'Pos (Item) <= 65535;
    end Is_Wide_Character;
    function Is_Wide_String (Item : Wide_Wide_String) return Boolean is
    begin
        for I in Item'Range loop
            if not Is_Wide_Character (Item (I)) then return False; end if;
        end loop;
        return True;
    end Is_Wide_String;
    function To_Wide_Character (Item : Character) return Wide_Character is
    begin
        return Wide_Character'Val (Character'Pos (Item));
    end To_Wide_Character;
    function To_Wide_String (Item : String) return Wide_String is
        Result : Wide_String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Wide_Character (Item (Item'First + I - 1));
        end loop;
        return Result;
    end To_Wide_String;
    function To_Wide_Wide_Character (Item : Character) return Wide_Wide_Character is
    begin
        return Wide_Wide_Character'Val (Character'Pos (Item));
    end To_Wide_Wide_Character;
    function To_Wide_Wide_String (Item : String) return Wide_Wide_String is
        Result : Wide_Wide_String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Wide_Wide_Character (Item (Item'First + I - 1));
        end loop;
        return Result;
    end To_Wide_Wide_String;
    function To_Wide_Wide_Character (Item : Wide_Character) return Wide_Wide_Character is
    begin
        return Wide_Wide_Character'Val (Wide_Character'Pos (Item));
    end To_Wide_Wide_Character;
    function To_Wide_Wide_String (Item : Wide_String) return Wide_Wide_String is
        Result : Wide_Wide_String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Wide_Wide_Character (Item (Item'First + I - 1));
        end loop;
        return Result;
    end To_Wide_Wide_String;
    function To_Character (Item : Wide_Character; Substitute : Character := ' ') return Character is
    begin
        if Wide_Character'Pos (Item) > 255 then return Substitute; end if;
        return Character'Val (Wide_Character'Pos (Item));
    end To_Character;
    function To_String (Item : Wide_String; Substitute : Character := ' ') return String is
        Result : String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Character (Item (Item'First + I - 1), Substitute);
        end loop;
        return Result;
    end To_String;
    function To_Character (Item : Wide_Wide_Character; Substitute : Character := ' ') return Character is
    begin
        if Wide_Wide_Character'Pos (Item) > 255 then return Substitute; end if;
        return Character'Val (Wide_Wide_Character'Pos (Item));
    end To_Character;
    function To_String (Item : Wide_Wide_String; Substitute : Character := ' ') return String is
        Result : String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Character (Item (Item'First + I - 1), Substitute);
        end loop;
        return Result;
    end To_String;
    function To_Wide_Character (Item : Wide_Wide_Character; Substitute : Wide_Character := ' ') return Wide_Character is
    begin
        if Wide_Wide_Character'Pos (Item) > 65535 then return Substitute; end if;
        return Wide_Character'Val (Wide_Wide_Character'Pos (Item));
    end To_Wide_Character;
    function To_Wide_String (Item : Wide_Wide_String; Substitute : Wide_Character := ' ') return Wide_String is
        Result : Wide_String (1 .. Item'Length);
    begin
        for I in Result'Range loop
            Result (I) := To_Wide_Character (Item (Item'First + I - 1), Substitute);
        end loop;
        return Result;
    end To_Wide_String;
end Ada.Characters.Conversions;
