with Ada.Strings; use Ada.Strings;
with Ada.Strings.Bounded;
with Ada.Strings.Maps;
with Ada.Text_IO;
with BoundedLibrary;
procedure BoundedStrings is
    package B is new Ada.Strings.Bounded.Generic_Bounded_Length (5);
    use B;
    package Small is new Ada.Strings.Bounded.Generic_Bounded_Length (1);
    s : Bounded_String;
    t : Bounded_String := To_Bounded_String ("abc");
    libraryText : BoundedLibrary.Bounded_String := BoundedLibrary.To_Bounded_String ("external");
    saved : Bounded_String;
    n : Natural;
    c : Character;
    first : Positive;
    last : Natural;
    mapping : Ada.Strings.Maps.Character_Mapping := Ada.Strings.Maps.To_Mapping ("ab", "AB");
    letters : Ada.Strings.Maps.Character_Set := Ada.Strings.Maps.To_Set ("ab");
    procedure check (condition : Boolean; message : String) is
    begin
        if not condition then
            Ada.Text_IO.Put_Line (message);
            raise Program_Error;
        end if;
    end check;
begin
    check (BoundedLibrary.To_String (libraryText) = "external", "library instance");
    BoundedLibrary.Delete (libraryText, 2, 7);
    check (BoundedLibrary.To_String (libraryText) = "el", "external generic body");
    check (Max_Length = 5 and Length_Range'Last = 5, "capacity");
    check (s = Null_Bounded_String and Length (s) = 0, "default initialization");
    check (t = "abc" and "abc" = t and t /= "ab", "mixed equality");
    s := t;
    Replace_Element (s, 2, 'X');
    check (t = "abc" and s = "aXc" and Element (s, 2) = 'X', "copy independence");
    check (t < "abd" and "abb" < t and t < To_Bounded_String ("abd"), "less");
    check (t <= "abc" and "abc" <= t and t <= t, "less equal");
    check (t > "abb" and "abd" > t and t > To_Bounded_String ("abb"), "greater");
    check (t >= "abc" and "abc" >= t and t >= t, "greater equal");
    check (To_Bounded_String ("abcdef", Drop => Left) = "bcdef", "drop left");
    check (To_Bounded_String ("abcdef", Drop => Right) = "abcde", "drop right");
    Set_Bounded_String (s, "ab");
    Append (s, 'c');
    Append (s, "d");
    Append (s, To_Bounded_String ("e"));
    check (s = "abcde", "append procedures");
    check (Append (s, 'f', Left) = "bcdef", "append drop");
    check (Append ("X", t) = "Xabc" and Append ('X', t) = "Xabc", "append left");
    check ((t & 'X') = "abcX" and ('X' & t) = "Xabc", "character concatenate");
    check ((t & "X") = "abcX" and ("X" & t) = "Xabc", "string concatenate");
    check ((t & To_Bounded_String ("X")) = "abcX", "bounded concatenate");
    declare
        part : String := Slice (s, 2, 4);
        empty : String := Slice (s, 6, 5);
    begin
        check (part = "bcd" and part'First = 2 and part'Last = 4, "slice retains bounds");
        check (empty'Length = 0 and empty'First = 6, "empty slice");
    end;
    Bounded_Slice (s, s, 2, 4);
    check (s = "bcd" and Bounded_Slice (s, 2, 3) = "cd", "bounded slices");
    s := To_Bounded_String ("ababa");
    check (Index (s, "aba") = 1 and Index (s, "aba", Backward) = 3, "pattern searches");
    check (Index (s, "aba", 4, Backward) = 1, "pattern From");
    check (Index (s, letters) = 1 and Index (s, letters, 3) = 3, "set searches");
    check (Count (s, "aba") = 1 and Count (s, letters) = 5, "count");
    check (Index_Non_Blank (s) = 1 and Index_Non_Blank (s, 2) = 2, "nonblank");
    Find_Token (s, letters, Inside, first, last);
    check (first = 1 and last = 5, "token");
    Find_Token (s, letters, 3, Inside, first, last);
    check (first = 3 and last = 5, "token From");
    check (Translate (s, mapping) = "ABABA", "translate function");
    Translate (s, mapping);
    check (s = "ABABA", "translate procedure");
    s := To_Bounded_String ("abc");
    check (Replace_Slice (s, 2, 2, "XY") = "aXYc", "replace function");
    Replace_Slice (s, 2, 2, "XY");
    Insert (s, 2, "!", Right);
    check (s = "a!XYc" and Insert (s, 1, "?", Left) = "a!XYc", "insert");
    Overwrite (s, 5, "zz", Right);
    check (s = "a!XYz" and Overwrite (s, 1, "b") = "b!XYz", "overwrite");
    Delete (s, 2, 4);
    check (s = "az" and Delete (s, 1, 2) = "", "delete");
    s := To_Bounded_String (" ab  ");
    check (Trim (s, Both) = "ab", "trim function");
    Trim (s, Both);
    s := Head (s, 5, '.');
    check (s = "ab...", "head function");
    Tail (s, 2);
    check (s = "..", "tail procedure");
    Head (s, 4, 'x');
    check (s = "..xx" and Tail (s, 5, '!') = "!..xx", "head procedure");
    Trim (s, Ada.Strings.Maps.To_Set ('.'), Ada.Strings.Maps.To_Set ('x'));
    check (s = "" and Trim (t, letters, letters) = "c", "set trims");
    check (Replicate (4, "ab", Left) = "babab", "replicate suffix");
    check (Replicate (Integer'Last, "ab", Right) = "ababa", "huge replication");
    check (Replicate (Integer'Last, "") = "", "empty replication");
    check (Replicate (8, 'X', Left) = "XXXXX", "character replication");
    check (Replicate (2, t, Right) = "abcab", "bounded replication");
    check ((2 * To_Bounded_String ("ab")) = "abab", "bounded multiplication");
    s := 3 * 'x';
    check (s = "xxx", "character multiplication");
    s := 2 * "ab";
    check (s = "abab", "string multiplication");
    check (Head (t, Integer'Last, '.', Left) = ".....", "huge head");
    check (Tail (t, Integer'Last, '.', Left) = "..abc", "huge tail");
    check (Small.To_String (Small.To_Bounded_String ("XY", Left)) = "Y", "second capacity");
    saved := s;
    for which in 1 .. 7 loop
        begin
            case which is
                when 1 => s := To_Bounded_String ("abcdef");
                when 2 => Append (s, "ZZ");
                when 3 => s := Replicate (Integer'Last, "abc");
                when 4 => Head (s, 6);
                when 5 => Tail (s, 6);
                when 6 => Insert (s, 2, "ZZ");
                when 7 => Overwrite (s, 5, "ZZ");
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when Length_Error => null;
        end;
        check (s = saved, "length failure preserves object");
    end loop;
    for which in 1 .. 4 loop
        begin
            case which is
                when 1 => c := Element (s, 5);
                when 2 => Replace_Element (s, 5, 'X');
                when 3 => s := Bounded_Slice (s, 6, 4);
                when 4 => s := Bounded_Slice (s, 1, 5);
                when others => raise Program_Error;
            end case;
            raise Program_Error;
        exception
            when Index_Error => null;
        end;
        check (s = saved, "index failure preserves object");
    end loop;
    Ada.Text_IO.Put_Line ("boundedstrings: passed");
end BoundedStrings;
