with StringMappingSupport; use StringMappingSupport;
with Ada.Strings; use Ada.Strings;
with Ada.Strings.Maps; use Ada.Strings.Maps;
with Ada.Strings.Maps.Constants; use Ada.Strings.Maps.Constants;
with Ada.Strings.Fixed;
with Ada.Strings.Bounded;
with Ada.Strings.Unbounded;
with Ada.Characters.Handling;
with Ada.Text_IO;
procedure StringMappingCallbacks is
    package F renames Ada.Strings.Fixed;
    package B is new Ada.Strings.Bounded.Generic_Bounded_Length (32);
    package U renames Ada.Strings.Unbounded;
    package H renames Ada.Characters.Handling;
    use type B.Bounded_String;
    use type U.Unbounded_String;
    mapping : Character_Mapping_Function := lower'Access;
    missing : Character_Mapping_Function := null;
    text : String (7 .. 12) := "ABABAB";
    bounded : B.Bounded_String := B.To_Bounded_String (text);
    unbounded : U.Unbounded_String := U.To_Unbounded_String (text);
    procedure check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error;
        end if;
    end check;
begin
    for c in Character loop
        check (Is_In (c, Control_Set) = H.Is_Control (c));
        check (Is_In (c, Graphic_Set) = H.Is_Graphic (c));
        check (Is_In (c, Letter_Set) = H.Is_Letter (c));
        check (Is_In (c, Lower_Set) = H.Is_Lower (c));
        check (Is_In (c, Upper_Set) = H.Is_Upper (c));
        check (Is_In (c, Basic_Set) = H.Is_Basic (c));
        check (Is_In (c, Decimal_Digit_Set) = H.Is_Digit (c));
        check (Is_In (c, Hexadecimal_Digit_Set) = H.Is_Hexadecimal_Digit (c));
        check (Is_In (c, Alphanumeric_Set) = H.Is_Alphanumeric (c));
        check (Is_In (c, Special_Set) = H.Is_Special (c));
        check (Is_In (c, ISO_646_Set) = H.Is_ISO_646 (c));
        check (Value (Lower_Case_Map, c) = H.To_Lower (c));
        check (Value (Upper_Case_Map, c) = H.To_Upper (c));
        check (Value (Basic_Map, c) = H.To_Basic (c));
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
        check (B.Count (B.Null_Bounded_String, "a", missing) = 0);
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
        fixedText : String (1 .. 0);
        boundedText : B.Bounded_String := B.Null_Bounded_String;
        unboundedText : U.Unbounded_String := U.Null_Unbounded_String;
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
        highText : String (Integer'Last - 3 .. Integer'Last) := "ABAB";
    begin
        check (F.Index (highText, "ab", Mapping => mapping) = Integer'Last - 3);
        check (F.Index (highText, "ab", Going => Backward, Mapping => mapping) = Integer'Last - 1);
        check (F.Count (highText, "ab", mapping) = 2);
        F.Translate (highText, mapping);
        check (highText = "abab");
    end;
    Ada.Text_IO.Put_Line ("stringmappingcallbacks: passed");
end StringMappingCallbacks;
