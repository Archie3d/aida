with Ada.Text_IO; use Ada.Text_IO;
with Callback_Library; use Callback_Library;
procedure Callbacklibrary is
    F : Function_Access := Double'Access;
    P : Procedure_Access := Set_Value'Access;
    X : Integer;
begin
    Put_Line (Integer'Image (F (5)));
    P.all (X);
    Put_Line (Integer'Image (X));
    if F = Choose then
        Put_Line ("same library callback");
    end if;
end Callbacklibrary;
