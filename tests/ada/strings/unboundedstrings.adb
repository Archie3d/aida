with Ada.Strings; use Ada.Strings;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Strings.Maps;
with Ada.Text_IO;
procedure UnboundedStrings is
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "unbounded string check";
        end if;
    end Check;
    A : Unbounded_String;
    B : Unbounded_String := To_Unbounded_String ("abc");
    C : Unbounded_String := B;
    Shifted : String (7 .. 9) := "xyz";
    P : String_Access := new String'(Shifted);
    High_Bound : String (Positive'Last .. Positive'Last) := "a";
    First : Positive;
    Last : Natural;
begin
    Check (Length (A) = 0 and A = Null_Unbounded_String);
    Check (To_String (A)'First = 1);
    Free (P);
    Check (P = null);
    Free (P);
    Set_Unbounded_String (A, High_Bound);
    Replace_Element (A, 1, 'b');
    Check (A = "b");
    Set_Unbounded_String (A, Shifted);
    Check (A = "xyz" and To_String (A)'First = 1);
    Replace_Element (C, 1, 'X');
    Check (C = "Xbc" and B = "abc");
    C := C;
    C := B;
    Append (B, B);
    Check (B = "abcabc" and C = "abc");
    Append (B, '!');
    Append (B, "?");
    Check (B = "abcabc!?" and Element (B, 7) = '!');
    Check (Slice (B, 2, 4) = "bca");
    Check (Slice (B, 9, 8) = "");
    Unbounded_Slice (B, C, 4, 6);
    Check (C = "abc" and Unbounded_Slice (B, 1, 3) = C);
    Check (('!' & C & '!' & "xx") = "!abc!xx");
    Check (("x" & C) = "xabc");
    Check (C < "b" and "a" < C and C <= "abc" and "abc" >= C);
    Check (C > "a" and C >= "abc" and "abc" <= C);
    Check (Count (B, "abc") = 2 and Index (B, "abc", 2) = 4);
    Check (Index (B, "abc", Backward) = 4);
    Check (Index (C, Ada.Strings.Maps.To_Set ('b')) = 2);
    Find_Token (C, Ada.Strings.Maps.To_Set ('b'), Inside, First, Last);
    Check (First = 2 and Last = 2);
    Check (Translate (C, Ada.Strings.Maps.To_Mapping ("ac", "AC")) = "AbC");
    Translate (C, Ada.Strings.Maps.To_Mapping ("ac", "AC"));
    Check (C = "AbC");
    C := To_Unbounded_String ("abc");
    Insert (C, 2, "XY");
    Check (C = "aXYbc");
    Overwrite (C, 2, "zz");
    Check (C = "azzbc");
    Replace_Slice (C, 2, 3, "Q");
    Check (C = "aQbc");
    Delete (C, 2, 3);
    Check (C = "ac");
    Check (Head (C, 4, '.') = "ac.." and Tail (C, 4, '.') = "..ac");
    Head (C, 3, ' ');
    Trim (C, Both);
    Check (C = "ac");
    Tail (C, 3);
    Trim (C, Ada.Strings.Maps.To_Set (' '), Ada.Strings.Maps.Null_Set);
    Check (C = "ac");
    Check ((3 * C) = "acacac" and (2 * 'x') = "xx" and (2 * "ab") = "abab");
    Check (Length (To_Unbounded_String (17)) = 17);
    for I in 1 .. 100 loop
        Append (A, "0123456789");
    end loop;
    Check (Length (A) = 1003 and Slice (A, 994, 1003) = "0123456789");
    begin
        Replace_Element (B, 9, 'x');
        raise Program_Error;
    exception
        when Index_Error => Check (B = "abcabc!?");
    end;
    begin
        C := Unbounded_Slice (B, 10, 8);
        raise Program_Error;
    exception
        when Index_Error => Check (C = "ac");
    end;
    begin
        declare
            Local : Unbounded_String := A;
        begin
            Append (Local, Local);
            raise Constraint_Error;
        end;
    exception
        when Constraint_Error => Check (Length (A) = 1003);
    end;
    Ada.Text_IO.Put_Line ("unbounded strings passed");
end UnboundedStrings;
