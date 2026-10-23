with Ada.Text_IO; use Ada.Text_IO;
with DiscriminantData; use DiscriminantData;
procedure DiscriminantTasking is
    type References is array (1 .. 4) of Ref;
    protected Store is
        procedure Next (Id : out Integer);
        procedure Publish (Id : Integer; P : Ref);
        function Get (Id : Integer) return Ref;
    private
        Counter : Integer := 0;
        Data : References;
    end Store;
    protected body Store is
        procedure Next (Id : out Integer) is
        begin
            Counter := Counter + 1;
            Id := Counter;
        end Next;
        procedure Publish (Id : Integer; P : Ref) is
        begin
            Data (Id) := P;
        end Publish;
        function Get (Id : Integer) return Ref is
        begin
            return Data (Id);
        end Get;
    end Store;
    task type Worker;
    type Workers is array (1 .. 4) of Worker;
    task body Worker is
        Id : Integer;
        X : Buffer;
        P : Ref;
        generic
            type Element is private;
        procedure Assign (A : in out Element; B : Element);
        procedure Assign (A : in out Element; B : Element) is
        begin
            A := B;
        end Assign;
        procedure Generic_Set is new Assign (Buffer);
        protected Gate is
            procedure Replace (Target : in out Buffer);
            entry Fill (Target : out Buffer);
        end Gate;
        protected body Gate is
            procedure Replace (Target : in out Buffer) is
            begin
                Target := Copy (Global);
            end Replace;
            entry Fill (Target : out Buffer) when True is
            begin
                Target := Copy (Global);
            end Fill;
        end Gate;
    begin
        Store.Next (Id);
        for Iteration in 1 .. 20 loop
            P := new Buffer (Id + Iteration);
            Generic_Set (X, P.all);
            X.Text (1) := 'z';
            if P.Text (1) /= 'x' or X.Text'Length /= Id + Iteration then
                raise Program_Error;
            end if;
            Set (X, Copy (Global));
            Gate.Replace (X);
            Gate.Fill (X);
            if X.Text /= "xxx" then raise Program_Error; end if;
            Free (P);
        end loop;
        P := new Buffer (Id);
        Store.Publish (Id, P);
    end Worker;
begin
    Bound := 8;
    if Global.Length /= 3 or Global.Text /= "xxx" then raise Program_Error; end if;
    declare
        Tasks : Workers;
    begin
        null;
    end;
    for Id in 1 .. 4 loop
        declare
            P : Ref := Store.Get (Id);
            R : Buffer := Copy (P.all);
        begin
            if P = null or P.Length /= Id or R.Text /= P.Text then raise Program_Error; end if;
            Free (P);
            if R.Text'Length /= Id then raise Program_Error; end if;
        end;
    end loop;
    Put_Line ("discriminant tasking ok");
end DiscriminantTasking;
