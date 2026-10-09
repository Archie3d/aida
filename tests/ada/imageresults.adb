with Ada.Text_IO; use Ada.Text_IO;
procedure Imageresults is
    type Fixed is delta 0.125 range -100.0 .. 100.0;
    type Item is (One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Eleven, Twelve,
                 An_Enumeration_Literal_Whose_Name_Is_Longer_Than_Sixty_Four_Characters_And_Must_Not_Be_Truncated);
    Kind : Integer := 0;
    function Expected (Value : Integer) return String is
    begin
        case Kind is
            when 0 => return Integer'Image (Value);
            when 1 => return Long_Integer'Image (Long_Integer (Value));
            when 2 => return Float'Image (Float (Value));
            when 3 => return Fixed'Image (Fixed (Value));
            when 4 => return Item'Image (Item'Val (Value - 1));
            when others => return Character'Image (Character'Val (64 + Value));
        end case;
    end Expected;
    procedure Validate (A, B, C, D, E, F, G, H, I, J, K, L : String) is
    begin
        if A /= Expected (1) or
           B /= Expected (2) or
           C /= Expected (3) or
           D /= Expected (4) or
           E /= Expected (5) or
           F /= Expected (6) or
           G /= Expected (7) or
           H /= Expected (8) or
           I /= Expected (9) or
           J /= Expected (10) or
           K /= Expected (11) or
           L /= Expected (12) then
            raise Program_Error with "image result overwritten";
        end if;
    end Validate;
begin
    Kind := 0;
    Validate (
        Integer'Image (1),
        Integer'Image (2),
        Integer'Image (3),
        Integer'Image (4),
        Integer'Image (5),
        Integer'Image (6),
        Integer'Image (7),
        Integer'Image (8),
        Integer'Image (9),
        Integer'Image (10),
        Integer'Image (11),
        Integer'Image (12));
    Kind := 1;
    Validate (
        Long_Integer'Image (1),
        Long_Integer'Image (2),
        Long_Integer'Image (3),
        Long_Integer'Image (4),
        Long_Integer'Image (5),
        Long_Integer'Image (6),
        Long_Integer'Image (7),
        Long_Integer'Image (8),
        Long_Integer'Image (9),
        Long_Integer'Image (10),
        Long_Integer'Image (11),
        Long_Integer'Image (12));
    Kind := 2;
    Validate (
        Float'Image (1.0),
        Float'Image (2.0),
        Float'Image (3.0),
        Float'Image (4.0),
        Float'Image (5.0),
        Float'Image (6.0),
        Float'Image (7.0),
        Float'Image (8.0),
        Float'Image (9.0),
        Float'Image (10.0),
        Float'Image (11.0),
        Float'Image (12.0));
    Kind := 3;
    Validate (
        Fixed'Image (1.0),
        Fixed'Image (2.0),
        Fixed'Image (3.0),
        Fixed'Image (4.0),
        Fixed'Image (5.0),
        Fixed'Image (6.0),
        Fixed'Image (7.0),
        Fixed'Image (8.0),
        Fixed'Image (9.0),
        Fixed'Image (10.0),
        Fixed'Image (11.0),
        Fixed'Image (12.0));
    Kind := 4;
    Validate (
        Item'Image (Item'Val (0)),
        Item'Image (Item'Val (1)),
        Item'Image (Item'Val (2)),
        Item'Image (Item'Val (3)),
        Item'Image (Item'Val (4)),
        Item'Image (Item'Val (5)),
        Item'Image (Item'Val (6)),
        Item'Image (Item'Val (7)),
        Item'Image (Item'Val (8)),
        Item'Image (Item'Val (9)),
        Item'Image (Item'Val (10)),
        Item'Image (Item'Val (11)));
    Kind := 5;
    Validate (
        Character'Image (Character'Val (65)),
        Character'Image (Character'Val (66)),
        Character'Image (Character'Val (67)),
        Character'Image (Character'Val (68)),
        Character'Image (Character'Val (69)),
        Character'Image (Character'Val (70)),
        Character'Image (Character'Val (71)),
        Character'Image (Character'Val (72)),
        Character'Image (Character'Val (73)),
        Character'Image (Character'Val (74)),
        Character'Image (Character'Val (75)),
        Character'Image (Character'Val (76)));
    declare
        Zero : String := Character'Image (Character'Val (0));
    begin
        if Zero'Length /= 3 or Zero (2) /= Character'Val (0) or Zero (3) /= Character'Val (39) then
            raise Program_Error with "embedded zero image";
        end if;
    end;
    if Item'Image (An_Enumeration_Literal_Whose_Name_Is_Longer_Than_Sixty_Four_Characters_And_Must_Not_Be_Truncated) /=
       "AN_ENUMERATION_LITERAL_WHOSE_NAME_IS_LONGER_THAN_SIXTY_FOUR_CHARACTERS_AND_MUST_NOT_BE_TRUNCATED" then
        raise Program_Error with "long enumeration image";
    end if;
    Put_Line ("image results ok");
end Imageresults;
