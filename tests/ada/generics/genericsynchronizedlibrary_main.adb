with Ada.Text_IO;
with GenericSynchronizedLibrary;
with GenericSynchronizedModel;
with Ada.Tags;
procedure GenericSynchronizedLibrary_Main is
    use type Ada.Tags.Tag;
    type Base is limited interface and GenericSynchronizedModel.Base;
    type Extra is synchronized interface and GenericSynchronizedModel.Extra;
    type Task_View is task interface and Extra and Base;
    type Protected_View is protected interface and Base and Extra;
    package First is new GenericSynchronizedLibrary (Base, Extra, Task_View);
    package Second is new GenericSynchronizedLibrary (Base, Extra, Protected_View);
    procedure Check is
    begin
        if First.Identity /= Task_View'Tag or Second.Identity /= Protected_View'Tag then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("library synchronized generic contracts passed");
end GenericSynchronizedLibrary_Main;
