with Ada.Finalization;
with Ada.Tags;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledLifetimes is
    Next_Id : Integer := 0;
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    procedure Initialize (Object : in out Guard) is
    begin
        if Object.Id /= 0 then
            raise Program_Error;
        end if;
        Next_Id := Next_Id + 1;
        Object.Id := Next_Id;
        Live := Live + 1;
        Put_Line ("initialize" & Integer'Image (Object.Id));
    end Initialize;
    procedure Finalize (Object : in out Guard) is
    begin
        Put_Line ("finalize" & Integer'Image (Object.Id));
        Live := Live - 1;
    end Finalize;
    type Child is new Guard with null record;
    subtype Child_View is Child;

    procedure Early (Take_Return : Boolean) is
    begin
        if Take_Return then
            return;
        end if;
        declare
            A, B : Guard;
        begin
            return;
        end;
    end Early;

    function Snapshot return Integer is
        Object : Guard;
    begin
        return Live;
    end Snapshot;

    procedure Recursive (Depth : Integer) is
        type Local is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (Object : in out Local) is
        begin
            Put_Line ("recursive" & Integer'Image (Depth));
        end Finalize;
        Object : Local;
    begin
        if Depth > 0 then
            Recursive (Depth - 1);
        end if;
    end Recursive;
begin
    if not Ada.Tags.Is_Abstract (Ada.Finalization.Controlled'Tag)
        or Ada.Tags.Is_Abstract (Guard'Tag) then
        raise Program_Error;
    end if;
    declare
        A : Guard;
        B : Child_View;
        Alias : Guard renames A;
    begin
        if Alias.Id /= 1 or Live /= 2 then
            raise Program_Error;
        end if;
        Put_Line ("block");
    end;
    Early (True);
    Early (False);
    for I in 1 .. 2 loop
        declare
            A : Guard;
        begin
            null;
        end;
    end loop;
    Outer : loop
        declare
            A : Guard;
        begin
            loop
                declare
                    B : Guard;
                begin
                    exit Outer when True;
                end;
            end loop;
        end;
    end loop Outer;
    if Live /= 0 then
        raise Program_Error;
    end if;
    if Snapshot /= 1 or Live /= 0 then
        raise Program_Error;
    end if;
    Recursive (2);
    Put_Line ("lifetimes passed");
end ControlledLifetimes;
