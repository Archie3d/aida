with Ada.Strings; use Ada.Strings;
with Ada.Strings.Fixed; use Ada.Strings.Fixed;
with Ada.Strings.Maps;
with Ada.Text_IO;
procedure FixedStringChecks is
    s : String (5 .. 7) := "abc";
    n : Natural;
    first : Positive;
    last : Natural;
    procedure check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error;
        end if;
    end check;
begin
    for which in 1 .. 4 loop
        begin
            case which is
                when 1 => n := Index (s, "");
                when 2 => n := Index ("", "", Backward);
                when 3 => n := Count (s, "");
                when 4 => n := Count ("", "");
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when Pattern_Error => null;
        end;
    end loop;
    for which in 1 .. 9 loop
        begin
            case which is
                when 1 => n := Index (s, "a", 4);
                when 2 => n := Index (s, Ada.Strings.Maps.Null_Set, 8);
                when 3 => n := Index_Non_Blank (s, 8);
                when 4 => Find_Token (s, Ada.Strings.Maps.Null_Set, 4, Inside, first, last);
                when 5 => s := Insert (s, 4, "");
                when 6 => s := Overwrite (s, 9, "");
                when 7 => s := Replace_Slice (s, 9, 8, "");
                when 8 => s := Replace_Slice (s, 5, 3, "");
                when 9 => s := Delete (s, 9, 10);
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when Index_Error => null;
        end;
        check (s = "abc");
    end loop;
    for which in 1 .. 6 loop
        begin
            case which is
                when 1 => Move ("abcd", s);
                when 2 => Move (" abc", s, Justify => Center);
                when 3 => Move ("abcd", s, Justify => Right);
                when 4 => Insert (s, 6, "X");
                when 5 => Replace_Slice (s, 6, 6, "XX");
                when 6 => Tail (s, 5);
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when Length_Error => null;
        end;
        check (s = "abc");
    end loop;
    begin
        s := Integer'Last * "ab";
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    check (Index ("", "a", 99) = 0);
    check (Index ("", Ada.Strings.Maps.Null_Set, 99) = 0);
    Ada.Text_IO.Put_Line ("fixedstringchecks: passed");
end FixedStringChecks;
