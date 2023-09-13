public class Foo
{
    fun Foo @op -> (Foo rhs)
    {
        this.setParent(rhs)
    }
    
    fun void setParent( Foo f )
    {
        //...
    }
    
    // the operator should be avaiable for use with class def
    fun Foo foo()
    {
        // wat...don't do this (but should compile)
        this -> foo();
    }
}

// this should fail because it's a duplicate overload
fun Foo @op ->(  Foo lhs,  Foo rhs )
{
    lhs.setParent( rhs );
}

//-----------------------------------------------------------------------------
// also operators can be overloaded from C++, hopefully,
// from chugins in the QUERY func (also as out-of-class overload)
// QUERY->add_overload( QUERY, Foo_operator_gruck, "Foo", "->" );
// QUERY->add_arg( QUERY, "Foo", "lhs" );
// QUERY->add_arg( QUERY, "Foo", "rhs" );
//-----------------------------------------------------------------------------
