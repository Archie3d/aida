with Ada.Text_IO; use Ada.Text_IO;
procedure InterfacePrivate is
    package Model is
        type I is synchronized interface;
        procedure Run (Self : in out I; N : out Integer) is abstract;
        type P is synchronized new I with private;
        type T is synchronized new I with private;
        type Ref is access all I'Class;
        function Current return Ref;
    private
        protected type P is new I with
            overriding procedure Run (N : out Integer);
        end P;
        task type T is new I with
            overriding entry Run (N : out Integer);
        end T;
    end Model;
    package body Model is
        protected body P is
            procedure Run (N : out Integer) is begin N := 42; end Run;
        end P;
        task body T is
        begin
            accept Run (N : out Integer) do N := 19; end Run;
        end T;
        function Current return Ref is begin return null; end Current;
    end Model;
    use Model;
    X : P;
    Worker : T;
    N : Integer;
    procedure Call (V : in out I'Class) is begin Run (V, N); end Call;
begin
    Call (Worker);
    if N /= 19 then raise Program_Error; end if;
    Call (X);
    Put_Line (Integer'Image (N));
end InterfacePrivate;
