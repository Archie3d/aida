with wideStringMappingSupport; use wideStringMappingSupport;
with Ada.Strings; use Ada.Strings;
with Ada.Strings.Wide_Maps; use Ada.Strings.Wide_Maps;
with Ada.Strings.Wide_Maps.Wide_Constants; use Ada.Strings.Wide_Maps.Wide_Constants;
with Ada.Strings.Wide_Fixed;
with Ada.Strings.Wide_Bounded;
with Ada.Strings.Wide_Unbounded;
with Ada.Characters.Handling;
with Ada.Wide_Text_IO;
procedure wideStringMappingCallbacks is
    package F renames Ada.Strings.Wide_Fixed;
    package B is new Ada.Strings.Wide_Bounded.Generic_Bounded_Length (32);
    package U renames Ada.Strings.Wide_Unbounded;
    package H renames Ada.Characters.Handling;
    use type B.Bounded_Wide_String;
    use type U.Unbounded_Wide_String;
    mapping : Wide_Character_Mapping_Function := lower'Access;
    missing : Wide_Character_Mapping_Function := null;
    text : Wide_String (7 .. 12) := "ABABAB";
    bounded : B.Bounded_Wide_String := B.To_Bounded_Wide_String (text);
    unbounded : U.Unbounded_Wide_String := U.To_Unbounded_Wide_String (text);
    procedure check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error;
        end if;
    end check;
begin
    for c in Wide_Character range Wide_Character'Val (0) .. Wide_Character'Val (255) loop
        check (Is_In (c, Control_Set) = H.Is_Control (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Graphic_Set) = H.Is_Graphic (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Letter_Set) = H.Is_Letter (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Lower_Set) = H.Is_Lower (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Upper_Set) = H.Is_Upper (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Basic_Set) = H.Is_Basic (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Decimal_Digit_Set) = H.Is_Digit (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Hexadecimal_Digit_Set) = H.Is_Hexadecimal_Digit (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Alphanumeric_Set) = H.Is_Alphanumeric (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, Special_Set) = H.Is_Special (Character'Val (Wide_Character'Pos (c))));
        check (Is_In (c, ISO_646_Set) = H.Is_ISO_646 (Character'Val (Wide_Character'Pos (c))));
        check (Value (Lower_Case_Map, c) = Wide_Character'Val (Character'Pos (H.To_Lower (Character'Val (Wide_Character'Pos (c))))));
        check (Value (Upper_Case_Map, c) = Wide_Character'Val (Character'Pos (H.To_Upper (Character'Val (Wide_Character'Pos (c))))));
        check (Value (Basic_Map, c) = Wide_Character'Val (Character'Pos (H.To_Basic (Character'Val (Wide_Character'Pos (c))))));
    end loop;
    check (F.Index (text, "ab", Mapping => mapping) = 7);
    check (calls = 2);
    check (F.Index (text, "ab", Going => Backward, Mapping => mapping) = 11);
    check (F.Index (text, "ab", 9, Mapping => mapping) = 9);
    check (F.Index (text, "ab", 10, Backward, mapping) = 9);
    check (F.Index (text, "AB", Mapping => mapping) = 0);
    check (F.Count (text, "aba", mapping) = 1);
    check (F.Count (text, "ab", mapping) = 3);
    check (F.Translate (text, mapping) = "ababab");
    check (B.Index (bounded, "ab", Mapping => mapping) = 1);
    check (B.Index (bounded, "ab", 3, Backward, mapping) = 1);
    check (B.Count (bounded, "ab", mapping) = 3);
    check (B.Translate (bounded, mapping) = "ababab");
    check (U.Index (unbounded, "ab", Mapping => mapping) = 1);
    check (U.Index (unbounded, "ab", 4, Backward, mapping) = 3);
    check (U.Count (unbounded, "ab", mapping) = 3);
    check (U.Translate (unbounded, mapping) = "ababab");
    fail := True;
    begin
        F.Translate (text, mapping);
        fail := False;
    exception
        when Program_Error => null;
    end;
    check (fail and text = "ABABAB");
    begin
        B.Translate (bounded, mapping);
        fail := False;
    exception
        when Program_Error => null;
    end;
    check (fail and bounded = "ABABAB");
    begin
        U.Translate (unbounded, mapping);
        fail := False;
    exception
        when Program_Error => null;
    end;
    check (fail and unbounded = "ABABAB");
    fail := False;
    F.Translate (text, mapping);
    B.Translate (bounded, mapping);
    U.Translate (unbounded, mapping);
    check (text = "ababab" and bounded = "ababab" and unbounded = "ababab");
    begin
        text := F.Translate ("", missing);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        check (F.Index ("", "a", Mapping => missing) = 0);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        check (B.Count (B.Null_Bounded_Wide_String, "a", missing) = 0);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        U.Translate (unbounded, missing);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        check (F.Count (text, "", mapping) = 0);
        raise Program_Error;
    exception
        when Pattern_Error => null;
    end;
    begin
        check (F.Index (text, "a", 1, Mapping => mapping) = 0);
        raise Program_Error;
    exception
        when Index_Error => null;
    end;
    check (F.Index ("", "a", 99, Mapping => mapping) = 0);
    check (F.Count ("a", "long", mapping) = 0);
    declare
        result : Natural;
        fixedText : Wide_String (1 .. 0);
        boundedText : B.Bounded_Wide_String := B.Null_Bounded_Wide_String;
        unboundedText : U.Unbounded_Wide_String := U.Null_Unbounded_Wide_String;
    begin
        for operation in 1 .. 15 loop
            begin
                case operation is
                    when 1 => result := F.Index ("", "a", Mapping => missing);
                    when 2 => result := F.Index ("", "a", 1, Mapping => missing);
                    when 3 => result := F.Count ("", "a", missing);
                    when 4 => fixedText := F.Translate ("", missing);
                    when 5 => F.Translate (fixedText, missing);
                    when 6 => result := B.Index (boundedText, "a", Mapping => missing);
                    when 7 => result := B.Index (boundedText, "a", 1, Mapping => missing);
                    when 8 => result := B.Count (boundedText, "a", missing);
                    when 9 => boundedText := B.Translate (boundedText, missing);
                    when 10 => B.Translate (boundedText, missing);
                    when 11 => result := U.Index (unboundedText, "a", Mapping => missing);
                    when 12 => result := U.Index (unboundedText, "a", 1, Mapping => missing);
                    when 13 => result := U.Count (unboundedText, "a", missing);
                    when 14 => unboundedText := U.Translate (unboundedText, missing);
                    when 15 => U.Translate (unboundedText, missing);
                    when others => null;
                end case;
                raise Program_Error;
            exception
                when Constraint_Error => null;
            end;
        end loop;
    end;
    declare
        highText : Wide_String (Integer'Last - 3 .. Integer'Last) := "ABAB";
    begin
        check (F.Index (highText, "ab", Mapping => mapping) = Integer'Last - 3);
        check (F.Index (highText, "ab", Going => Backward, Mapping => mapping) = Integer'Last - 1);
        check (F.Count (highText, "ab", mapping) = 2);
        F.Translate (highText, mapping);
        check (highText = "abab");
    end;
    Ada.Wide_Text_IO.Put_Line ("wideStringMappingCallbacks: passed");
end wideStringMappingCallbacks;
