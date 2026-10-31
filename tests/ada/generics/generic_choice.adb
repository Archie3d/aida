function Generic_Choice (Left, Right : Element) return Element is
begin
    if "<" (Left, Right) then
        return Left;
    end if;
    return Right;
end Generic_Choice;
