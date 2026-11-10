with Concurrent_View_Pkg; use Concurrent_View_Pkg;
with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentViewLibraryMain is
    generic with procedure Op (Value : in out Integer);
    procedure Apply (Value : in out Integer);
    procedure Apply (Value : in out Integer) is
    begin Op (Value); end Apply;
    procedure Run is new Apply (Bump);
    X, Y : Integer := 0;
    Saved : Action := Callback;
begin
    Read (Y);
    Run (X);
    Saved (X);
    Callback (X);
    if Default_Object.Get /= 11 or X /= 9 or Y /= 7 or Object.N /= 3 or Object.Value /= 3 then raise Program_Error; end if;
    Put_Line ("concurrent library OK");
end ConcurrentViewLibraryMain;
