with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentViewDefaults is
    protected type Gate (N : Integer) is procedure Add (X : in out Integer); function Get return Integer; end Gate;
    protected body Gate is
        procedure Add (X : in out Integer) is begin X := X + N; end Add;
        function Get return Integer is begin return N; end Get;
    end Gate;
    A : aliased Gate (3);
    B : aliased Gate (5);
    type Ref is access all Gate;
    P : Ref := A'Access;
    generic with procedure Op (V : in out Integer) is P.Add;
    procedure Apply (V : in out Integer);
    procedure Apply (V : in out Integer) is begin Op (V); end Apply;
    procedure One is new Apply;
    procedure Renamed (V : in out Integer) renames P.Add;
    type Callback is access protected procedure (V : in out Integer);
    C : Callback := Renamed'Access;
    X : Integer := 0;
begin
    P := B'Access;
    declare procedure Two is new Apply;
    begin One (X); Two (X); C (X); end;
    if X /= 11 then raise Program_Error; end if;
    declare
        type Getter is access protected function return Integer;
        function Get return Integer renames A.Get;
        Read : Getter := Get'Access;
        function Create return Callback is begin return Renamed'Access; end Create;
        Bound : Callback := Create;
    begin
        if Read.all /= 3 then raise Program_Error; end if;
        Bound (X);
        if X /= 14 then raise Program_Error; end if;
    end;
    Put_Line ("concurrent defaults OK");
end ConcurrentViewDefaults;
