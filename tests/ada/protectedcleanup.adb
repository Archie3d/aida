with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure ProtectedCleanup is
    failInitialization : Boolean := False;
    function Initial return Integer is
    begin
        if failInitialization then
            raise Constraint_Error;
        end if;
        return 1;
    end Initial;
    function Fail return Integer is
    begin
        raise Constraint_Error;
        return 0;
    end Fail;
    protected type Guard is
        function Get return Integer;
    private
        value : Integer := Initial;
    end Guard;
    protected body Guard is
        function Get return Integer is
        begin
            return value;
        end Get;
    end Guard;
    type Broken is record
        initialized : Guard;
        failure : Integer := Fail;
    end record;
    type Pointer is access Guard;
    procedure Free is new Ada.Unchecked_Deallocation (Guard, Pointer);
    object : Pointer;
begin
    for iteration in 1 .. 10 loop
        begin
            declare
                failed : Broken;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        failInitialization := True;
        begin
            object := new Guard;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        failInitialization := False;
        object := new Guard;
        if object.Get /= 1 then
            raise Program_Error;
        end if;
        Free (object);
    end loop;
    Ada.Text_IO.Put_Line ("protected cleanup passed");
end ProtectedCleanup;
