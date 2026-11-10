with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentViews is
    protected type Gate (N : Positive := 3) is
        entry Add (V : in out Integer);
        procedure Inc (V : in out Integer);
        procedure Again (V : in out Integer);
        function Get return Integer;
    private Total : Integer := N; end Gate;
    protected body Gate is
        entry Add (V : in out Integer) when True is begin Total := Total + 1; V := V + N; end Add;
        procedure Inc (V : in out Integer) is begin V := V + N; end Inc;
        procedure Again (V : in out Integer) is
            procedure Local (X : in out Integer) renames Inc;
            generic with procedure Op (X : in out Integer);
            procedure Forward (X : in out Integer);
            procedure Forward (X : in out Integer) is begin Op (X); end Forward;
            procedure Run_Local is new Forward (Inc);
        begin Local (V); Run_Local (V); end Again;
        function Get return Integer is begin return Total; end Get;
    end Gate;
    A, B : aliased Gate;
    type Ref is access all Gate;
    P : Ref := A'Access;
    procedure Add (X : in out Integer) renames P.Add;
    procedure Inc (X : in out Integer) renames P.Inc;
    function Get return Integer renames P.Get;
    type Callback is access protected procedure (X : in out Integer);
    C : Callback := A.Inc'Access;
    generic with procedure Op (N : in out Integer);
    procedure Apply (N : in out Integer);
    procedure Apply (N : in out Integer) is begin Op (N); end Apply;
    procedure Run is new Apply (A.Add);
    task Server is entry E (1 .. 2) (N : in out Integer); end Server;
    task body Server is begin accept E (2) (N : in out Integer) do N := N + 5; end E; end Server;
    Index : Integer := 2;
    procedure Send (N : in out Integer) renames Server.E (Index);
    X : Integer := 0;
begin
    P := B'Access;
    Index := 1;
    Add (X); Inc (X); Run (X); C (X);
    select Send (X); or delay 2.0; raise Program_Error; end select;
    if X /= 17 or Get /= 5 or A.Get /= 5 or B.Get /= 3 then raise Program_Error; end if;
    A.Again (X);
    declare
        D : Callback := A.Inc'Access;
        E : Callback := B.Inc'Access;
        type Other_Callback is access protected procedure (X : in out Integer);
        Converted : Other_Callback := Other_Callback (C);
        type Holder is record Op : Callback; end record;
        Left : Holder := (Op => C);
        Right : Holder := (Op => D);
        type Callbacks is array (1 .. 2) of Callback;
        First : Callbacks := (C, D);
        Second : Callbacks := (D, C);
        generic type Formal is access protected procedure (X : in out Integer);
        procedure Apply_Callback (Op : Formal; Value : in out Integer);
        procedure Apply_Callback (Op : Formal; Value : in out Integer) is begin Op (Value); end Apply_Callback;
        procedure Generic_Call is new Apply_Callback (Callback);
        procedure Invoke (Op : not null access protected procedure (X : in out Integer)) is
            Alias : access protected procedure (V : in out Integer) renames Op;
        begin Alias (X); end Invoke;
    begin
        if C /= D or C = E or C = null then raise Program_Error; end if;
        if Left /= Right or First /= Second then raise Program_Error; end if;
        Generic_Call (C, X);
        Invoke (Converted);
        if X /= 29 then raise Program_Error; end if;
        D := null;
        begin D (X); raise Program_Error;
        exception when Constraint_Error => null;
        end;
    end;
    begin
        declare
            Missing : Ref := null;
            procedure Invalid (X : in out Integer) renames Missing.Add;
        begin raise Program_Error;
        end;
    exception when Constraint_Error => null;
    end;
    Put_Line ("concurrent views OK");
end ConcurrentViews;
