procedure SharedSyntaxErrors is
    value : Integer := 0;
    pragma Atomic (value, value);
    pragma Volatile (Entity => value);
    pragma Atomic_Components (value);
    pragma Volatile_Components (value);
begin
    pragma Atomic (value);
    null;
end SharedSyntaxErrors;
