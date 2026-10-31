with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Fixed_IO;
procedure Fixedtext is
    type Coarse is delta 0.125 range -100.0 .. 100.0;
    type Fine is delta 0.01 range -100.0 .. 100.0;
    type Whole is delta 1.0 range -9223372036854775808.0 .. 9223372036854775807.0;
    subtype Narrow is Coarse range -1.0 .. 1.0;
    package C_IO is new Ada.Text_IO.Fixed_IO (Coarse);
    package F_IO is new Ada.Text_IO.Fixed_IO (Fine);
    package W_IO is new Ada.Text_IO.Fixed_IO (Whole);
    package N_IO is new Ada.Text_IO.Fixed_IO (Narrow);
    X : Coarse;
    Small_X : Narrow;
    Y : Fine;
    W : Whole;
    Last : Positive;
    Text : String (5 .. 16);
    File : File_Type;
    Count : Natural := 0;
    procedure Bad_Value (Text : String) is
    begin
        X := Coarse'Value (Text);
        raise Program_Error;
    exception
        when Constraint_Error => Count := Count + 1;
    end Bad_Value;
    procedure Bad_Input (Text : String) is
    begin
        C_IO.Get (Text, X, Last);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end Bad_Input;
begin
    if Coarse'Aft /= 1 or Fine'Aft /= 2 or Coarse'Fore /= 4 or Narrow'Fore /= 2 then
        raise Program_Error;
    end if;
    if Coarse'Image (1.25) /= " 1.3" or Coarse'Image (-1.25) /= "-1.3"
        or Fine'Image (0.125) /= " 0.13" or Whole'Image (Whole'Last) /= " 9223372036854775807.0"
        or Whole'Image (Whole'First) /= "-9223372036854775808.0" then
        raise Program_Error;
    end if;
    if Coarse'Value ("  +0.0625  ") /= 0.125
        or Coarse'Value ("-0.0625") /= -0.125
        or Coarse'Value ("0.06249999999999999999999999999999999999999999") /= 0.0
        or Coarse'Value ("16#0.1#") /= 0.125
        or Fine'Value ("2#1.01#E+1") /= 2.5
        or Coarse'Value ("1_0.0E-1") /= 1.0
        or Coarse'Value ("1e-9999999999999999999999") /= 0.0 then
        raise Program_Error;
    end if;
    if Coarse'Value (".125") /= 0.125 or Coarse'Value ("1.") /= 1.0
        or Coarse'Value ("16#.2#") /= 0.125 or Narrow'Value ("1.125") /= 1.125 then
        raise Program_Error;
    end if;
    W := Whole'Value ("9223372036854775807");
    if W /= Whole'Last or Whole'Value (Whole'Image (Whole'First)) /= Whole'First then
        raise Program_Error;
    end if;
    C_IO.Put (Text, 1.25, Aft => 3);
    if Text /= "       1.250" then raise Program_Error; end if;
    C_IO.Put (Text, -1.25, Aft => 1, Exp => 3);
    if Text /= "    -1.3E+00" then raise Program_Error; end if;
    F_IO.Put (Text, 9.999, Aft => 1, Exp => 2);
    if Text /= "      1.0E+1" then raise Program_Error; end if;
    C_IO.Get ("  .0625,rest", X, Last);
    if X /= 0.125 or Last /= 7 then raise Program_Error; end if;
    Text := "  1.25, rest";
    C_IO.Get (Text, X, Last);
    if X /= 1.25 or Last /= 10 then raise Program_Error; end if;
    F_IO.Get ("16#A.#", Y, Last);
    if Y /= 10.0 or Last /= 6 then raise Program_Error; end if;
    W_IO.Get ("-9223372036854775808", W, Last);
    if W /= Whole'First then raise Program_Error; end if;
    Bad_Value ("");
    Bad_Value ("1__0.0");
    Bad_Value ("1.0junk");
    Bad_Value ("nan");
    Bad_Value ("1e+");
    Bad_Value ("2#2.0#");
    Bad_Value ("100.125");
    Bad_Value ("1e9999999999999999999999");
    Bad_Input ("1__2");
    Bad_Input ("100.125");
    Bad_Input ("inf");
    begin
        Small_X := Narrow'Value ("1.125");
        raise Program_Error;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        N_IO.Get ("1.125", X, Last);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    begin
        C_IO.Put (Text (5 .. 6), 1.0);
        raise Program_Error;
    exception
        when Layout_Error => Count := Count + 1;
    end;
    if Count /= 14 then raise Program_Error; end if;

    Create (File, Out_File, "fixedtext.tmp");
    C_IO.Put (File, 1.25, Fore => 0, Aft => 3);
    Put (File, ",");
    C_IO.Put (File, -2.5, Fore => 0, Aft => 3);
    New_Line (File);
    Put_Line (File, "  1.250 2.500");
    Reset (File, In_File);
    C_IO.Get (File, X);
    if X /= 1.25 then raise Program_Error; end if;
    declare
        Separator : Character;
    begin
        Get (File, Separator);
        if Separator /= ',' then raise Program_Error; end if;
    end;
    C_IO.Get (File, X);
    if X /= -2.5 then raise Program_Error; end if;
    Skip_Line (File);
    C_IO.Get (File, X, Width => 7);
    if X /= 1.25 then raise Program_Error; end if;
    Set_Input (File);
    C_IO.Get (X);
    if X /= 2.5 then raise Program_Error; end if;
    Set_Input (Standard_Input);
    Delete (File);
    C_IO.Put (1.25, Fore => 0, Aft => 3);
    New_Line;
    Put_Line ("fixed text: exact rounding, formatting, parsing, and exceptions");
end Fixedtext;
