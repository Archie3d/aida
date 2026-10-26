with Ada.Text_IO;
with Ada.Tags;
procedure GenericSynchronizedNested is
    use type Ada.Tags.Tag;
    type Base is limited interface;
    type Sync is synchronized interface and Base;
    type Task_View is task interface and Sync;
    type Protected_View is protected interface and Sync;
    generic
        type T (<>) is abstract synchronized new Base with private;
    package Outer is
        subtype Alias is T;
        generic
            type U is abstract synchronized new Alias with private;
        function Identity return Ada.Tags.Tag;
    end Outer;
    package body Outer is
        function Identity return Ada.Tags.Tag is
        begin
            return U'Tag;
        end Identity;
    end Outer;
    package First is new Outer (Sync);
    package Second is new Outer (Sync);
    function Task_Tag is new First.Identity (Task_View);
    function Protected_Tag is new Second.Identity (Protected_View);
    procedure Check is
    begin
        if Task_Tag /= Task_View'Tag or Protected_Tag /= Protected_View'Tag then
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
    Ada.Text_IO.Put_Line ("nested synchronized generic contracts passed");
end GenericSynchronizedNested;
