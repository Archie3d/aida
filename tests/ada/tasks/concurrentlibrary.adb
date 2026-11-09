package body ConcurrentLibrary is
    task body Worker is begin null; end Worker;
    protected body Guard is
        procedure Set (N : Integer) is begin Guard.N := N; end Set;
        function Get return Integer is begin return N; end Get;
    end Guard;
    function MakeGuard return Guard is
    begin
        return Object : Guard do Object.Set (5); end return;
    end MakeGuard;
    function Make return Result is
    begin
        return (W => <>, G => MakeGuard);
    end Make;
    function Value (Object : Result) return Integer is
    begin
        return Object.G.Get;
    end Value;
end ConcurrentLibrary;
