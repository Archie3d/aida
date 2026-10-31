with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Fixed_IO;
procedure Fixedtextchecks is
    type Number is delta 0.01 range -10.0 .. 10.0;
    package IO is new Ada.Text_IO.Fixed_IO (Number);
    File : File_Type;
    X : Number;
    Last : Positive;
    Count : Natural := 0;
    Buffer : String (1 .. 8);
    High_Bounds : String (2147483645 .. 2147483647) := "1.0";
    generic
        type Num is delta <>;
    function Show (Value : Num) return String;
    function Show (Value : Num) return String is
    begin
        return Num'Image (Value);
    end Show;
    function Image is new Show (Number);
begin
    if Image (1.25) /= " 1.25" then raise Program_Error; end if;
    IO.Get (High_Bounds, X, Last);
    if X /= 1.0 or Last /= 2147483647 then raise Program_Error; end if;
    IO.Default_Aft := 3;
    IO.Put (Buffer, 1.25);
    if Buffer /= "   1.250" then raise Program_Error; end if;
    begin
        IO.Get ("", X, Last);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    begin
        IO.Get (File, X);
        raise Program_Error;
    exception
        when Status_Error => Count := Count + 1;
    end;
    Create (File, Out_File, "fixedtextchecks.tmp");
    begin
        IO.Get (File, X);
        raise Program_Error;
    exception
        when Mode_Error => Count := Count + 1;
    end;
    Set_Output (File);
    IO.Put (1.25, Fore => 0);
    Put ("-2.500.5");
    New_Line;
    Put_Line ("  ");
    Put_Line ("1.25oops");
    Put_Line ("10.125");
    Put_Line ("1e+");
    Put_Line ("16#A.0#");
    Set_Output (Standard_Output);
    Reset (File, In_File);
    begin
        IO.Put (File, 1.0);
        raise Program_Error;
    exception
        when Mode_Error => Count := Count + 1;
    end;
    IO.Get (File, X);
    if X /= 1.25 then raise Program_Error; end if;
    IO.Get (File, X);
    if X /= -2.5 then raise Program_Error; end if;
    IO.Get (File, X);
    if X /= 0.5 then raise Program_Error; end if;
    Skip_Line (File);
    begin
        IO.Get (File, X, Width => 2);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    Skip_Line (File);
    begin
        IO.Get (File, X, Width => 8);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    Skip_Line (File);
    begin
        IO.Get (File, X);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    begin
        IO.Get (File, X);
        raise Program_Error;
    exception
        when Data_Error => Count := Count + 1;
    end;
    IO.Get (File, X, Width => 0);
    if X /= 10.0 then raise Program_Error; end if;
    begin
        IO.Get (File, X);
        raise Program_Error;
    exception
        when End_Error => Count := Count + 1;
    end;
    Delete (File);
    if Count /= 9 then raise Program_Error; end if;
    Put_Line ("fixed I/O: defaults, field widths, token boundaries, and file exceptions");
end Fixedtextchecks;
