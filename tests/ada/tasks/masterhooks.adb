with Ada.Finalization;
with Master_Model; use Master_Model;
procedure Masterhooks is
    Main_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (2));
    procedure Early_Return is
        Outer_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (3));
    begin
        declare
            Middle_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (13));
        begin
            declare
                Inner_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (4));
            begin
                return;
            end;
        end;
    end Early_Return;
    procedure Plain (Mode : Integer) is
    begin
        for I in 1 .. 2 loop
            begin
                if Mode = 0 then
                    return;
                elsif Mode = 1 then
                    exit;
                else
                    raise Constraint_Error;
                end if;
            end;
        end loop;
    exception
        when Constraint_Error => null;
    end Plain;
    procedure Unhandled is
    begin
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (5));
        begin
            raise Constraint_Error with "preserved";
        end;
    end Unhandled;
    procedure Rethrow is
        G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (14));
    begin
        declare
            Inner : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (15));
        begin
            raise Constraint_Error;
        exception
            when Constraint_Error => raise;
        end;
    exception
        when Constraint_Error => Check_Done (15);
    end Rethrow;
    function Missing_Return return Integer is
        G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (16));
    begin
        null;
    end Missing_Return;
    function Build return Guard is
    begin
        return Result : Guard do
            declare
                G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (17));
            begin
                return;
            end;
        end return;
    end Build;
    function Wide return Guard'Class is
    begin
        return Result : Guard;
    end Wide;
begin
    Early_Return;
    Check_Done (4);
    Check_Done (13);
    Check_Done (3);
    Plain (0);
    Plain (1);
    Plain (2);
    begin
        Unhandled;
    exception
        when Constraint_Error => Check_Done (5);
    end;
    for I in 1 .. 2 loop
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (6));
        begin
            exit when I = 1;
        end;
    end loop;
    Check_Done (6);
    declare
        G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (7));
    begin
        raise Constraint_Error;
    exception
        when Constraint_Error => null;
    end;
    Check_Done (7);
    begin
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (8));
            Bad : Integer := Fail;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Check_Done (8);
    end;
    declare
        type Pointer is access Guard;
        P : Pointer := new Guard;
    begin
        P.Id := Remember_Allocation (9);
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (10));
        begin
            null;
        end;
        Check_Done (10);
    end;
    Check_Done (9);
    declare
        type Link is access Guard'Class;
        P : Link := new Guard'Class'(Wide);
    begin
        P.Id := Remember_Allocation (19);
    end;
    Check_Done (19);
    declare
        type Soft is new Ada.Finalization.Controlled with record
            Id : Integer := 0;
        end record;
        overriding procedure Finalize (Object : in out Soft) is
        begin
            if Object.Id /= 0 then
                Finalized (Object.Id);
            end if;
        end Finalize;
        type Link is access Soft'Class;
        Source : Soft;
        P : Link := new Soft'Class'(Source);
    begin
        P.Id := Remember_Allocation (20);
    end;
    Check_Done (20);
    Rethrow;
    Check_Done (14);
    begin
        declare
            Value : Integer := Missing_Return;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check_Done (16);
    end;
    declare
        Result : Guard := Build;
    begin
        Check_Done (17);
    end;
    begin
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (18));
        begin
            raise Program_Error;
        end;
    exception
        when Tasking_Error => Check_Done (18);
    end;
    begin
        declare
            G : Guard := (Ada.Finalization.Limited_Controlled with Id => Remember (12));
        begin
            null;
        end;
    exception
        when Program_Error => Check_Done (12);
    end;
end Masterhooks;
