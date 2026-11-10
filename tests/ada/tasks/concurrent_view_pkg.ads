package Concurrent_View_Pkg is
    protected type Counter (N : Positive := 2) is
        procedure Add (X : in out Integer);
        function Value return Integer;
    private
        Text : String (1 .. N) := (others => 'x');
    end Counter;
    Object : aliased Counter (3);
    procedure Bump (X : in out Integer) renames Object.Add;
    type Action is access protected procedure (X : in out Integer);
    function Callback return Action;
    protected type With_Default (Value : access Integer := new Integer'(11)) is
        function Get return Integer;
    end With_Default;
    Default_Object : With_Default;
    task type Worker (N : Integer) is
        entry Read (Value : out Integer);
    end Worker;
    Server : Worker (7);
    procedure Read (Value : out Integer) renames Server.Read;
end Concurrent_View_Pkg;
