with Ada.Text_IO; use Ada.Text_IO;
with ConcurrentLibrary; use ConcurrentLibrary;
procedure ConcurrentLibraryMain is
    type Factory is access function return Result;
    Constructor : Factory := Make'Access;
    function Forward return Result is begin return Constructor.all; end Forward;
    Object : Result := Forward;
begin
    if Value (Object) /= 5 then raise Program_Error; end if;
    Put_Line ("library results OK");
end ConcurrentLibraryMain;
