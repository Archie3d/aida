with Ada.Text_IO; use Ada.Text_IO;
procedure DefaultDiscriminantsTasking is
    protected Results is
        procedure Done;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Total := Total + 1;
        end Done;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
    procedure Worker is
        type Item (D : Boolean := False) is record
            case D is
                when False => A : Integer := 1;
                when True => B : Integer := 2;
            end case;
        end record;
        procedure Replace (Target : in out Item) is
        begin
            if Target'Constrained then
                raise Program_Error;
            end if;
            Target := (True, 12);
        end Replace;
        type Callback is access procedure (Target : in out Item);
        Action : Callback := Replace'Access;
        generic
            type Element is private;
        procedure Assign (Target : in out Element; Source : Element);
        procedure Assign (Target : in out Element; Source : Element) is
        begin
            Target := Source;
        end Assign;
        procedure Copy is new Assign (Item);
        protected Gate is
            procedure Set (Target : in out Item);
            entry Fill (Target : out Item);
        end Gate;
        protected body Gate is
            procedure Set (Target : in out Item) is
            begin
                if Target'Constrained then
                    raise Program_Error;
                end if;
                Target := (False, 9);
            end Set;
            entry Fill (Target : out Item) when True is
            begin
                if Target'Constrained then
                    raise Program_Error;
                end if;
                Target := (True, 15);
            end Fill;
        end Gate;
        X : Item;
        Y : Item := (False, 8);
    begin
        for N in 1 .. 50 loop
            Action (X);
            if not X.D or X.B /= 12 then
                raise Program_Error;
            end if;
            Copy (X, Y);
            if X.D or X.A /= 8 then
                raise Program_Error;
            end if;
            Gate.Set (X);
            Gate.Fill (X);
            if not X.D or X.B /= 15 then
                raise Program_Error;
            end if;
        end loop;
        Results.Done;
    end Worker;
    task type Runner;
    type Runners is array (1 .. 4) of Runner;
    task body Runner is
    begin
        Worker;
    end Runner;
begin
    declare
        Tasks : Runners;
    begin
        null;
    end;
    if Results.Count /= 4 then
        raise Program_Error;
    end if;
    Put_Line ("task discriminants ok");
end DefaultDiscriminantsTasking;
