with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Modular_IO;
procedure ModularIO is
    type Word is mod 2 ** 64;
    package IO is new Ada.Text_IO.Modular_IO (Word);
    X : Word := Word'Last;
    Text : String (1 .. 24);
    Last : Positive;
    F : File_Type;
    Ch : Character;
    Count : Integer := 0;
    Short : String (1 .. 2);
begin
    IO.Put (X, Width => 0);
    New_Line;
    IO.Put (X, Width => 0, Base => 16);
    New_Line;
    IO.Put (Text, X);
    if Text /= "    18446744073709551615" then
        raise Program_Error;
    end if;
    IO.Get (" 16#FFFF_FFFF_FFFF_FFFF# tail", X, Last);
    if X /= Word'Last or Last /= 24 then
        raise Program_Error;
    end if;
    Create (F, Out_File, "modular.tmp");
    IO.Put (F, X, Width => 0);
    New_Line (F);
    Reset (F, In_File);
    IO.Get (F, X);
    Delete (F);
    if X /= Word'Last then
        raise Program_Error;
    end if;
    IO.Get ("123xyz", X, Last);
    if X /= 123 or Last /= 3 then
        raise Program_Error;
    end if;
    begin
        IO.Get ("18446744073709551616", X, Last);
    exception
        when Data_Error => Count := Count + 1;
    end;
    begin
        IO.Put (Short, 1000);
    exception
        when Layout_Error => Count := Count + 1;
    end;
    if X /= 123 or Count /= 2 then
        raise Program_Error;
    end if;
    Create (F, Out_File, "modular.tmp");
    Put_Line (F, " 16#FF#,12");
    Put_Line (F, "12345");
    Put_Line (F, "99");
    Put_Line (F, "-0");
    Put (F, "2#");
    for I in 1 .. 300 loop
        Put (F, '0');
    end loop;
    Put_Line (F, "1#");
    Reset (F, In_File);
    IO.Get (F, X);
    Get (F, Ch);
    if X /= 255 or Ch /= ',' then
        raise Program_Error;
    end if;
    IO.Get (F, X);
    Skip_Line (F);
    if X /= 12 then
        raise Program_Error;
    end if;
    IO.Get (F, X, Width => 3);
    if X /= 123 then
        raise Program_Error;
    end if;
    IO.Get (F, X, Width => 20);
    if X /= 45 then
        raise Program_Error;
    end if;
    Skip_Line (F);
    IO.Get (F, X, Width => 20);
    Skip_Line (F);
    if X /= 99 then
        raise Program_Error;
    end if;
    begin
        IO.Get (F, X);
    exception
        when Data_Error => Count := Count + 1;
    end;
    if X /= 99 or Count /= 3 then
        raise Program_Error;
    end if;
    IO.Get (F, X);
    if X /= 1 then
        raise Program_Error;
    end if;
    Delete (F);
    Put_Line ("modular IO passed");
end ModularIO;
