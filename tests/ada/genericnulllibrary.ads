generic
    type T is private;
    Default_Amount : Integer := 3;
    with procedure Hook (Object : in out T; Amount : Integer := Default_Amount) is null;
package GenericNullLibrary is
    procedure Invoke (Object : in out T);
    procedure Renamed (Object : in out T; Amount : Integer := Default_Amount) renames Hook;
    generic
        with procedure Action (Object : in out T; Amount : Integer := 4) is Hook;
    package Inner is
        procedure Invoke (Object : in out T);
    end Inner;
end GenericNullLibrary;
