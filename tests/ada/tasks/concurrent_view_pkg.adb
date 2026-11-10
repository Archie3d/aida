package body Concurrent_View_Pkg is
    protected body Counter is
        procedure Add (X : in out Integer) is
        begin X := X + N; end Add;
        function Value return Integer is
        begin return Text'Length; end Value;
    end Counter;
    function Callback return Action is
    begin return Bump'Access; end Callback;
    protected body With_Default is
        function Get return Integer is begin return Value.all; end Get;
    end With_Default;
    task body Worker is
        function Result return Integer is
        begin return Worker.N; end Result;
    begin
        accept Read (Value : out Integer) do
            Value := Result;
        end Read;
    end Worker;
end Concurrent_View_Pkg;
