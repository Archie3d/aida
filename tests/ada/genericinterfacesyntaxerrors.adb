generic
    type F (<>) is interface;
package Unknown_Discriminants is
end Unknown_Discriminants;
generic
    type F is abstract interface;
package Explicit_Abstract is
end Explicit_Abstract;
generic
    type F is interface and;
package Missing_Ancestor is
end Missing_Ancestor;
generic
    type F is synchronized new Integer with private;
package Synchronized_Derived is
end Synchronized_Derived;
