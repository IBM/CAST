import React, { Component } from 'react';
import { render, unmountComponentAtNode } from 'react-dom';


class CASTSearchVisComponent extends Component {
    constructor(props){
        super(props)
        this.state = {
            aid : 0,
            jid : 0,
            sid : 0
        }
    }

    searchAllocation(event){
        console.log(event);
    }

    
    searchJob(event){
        
    }

    handleChange(propertyName, event) {
        this.state[propertyName] = event.target.value;
        console.log(this.state);
    }

    render() {
        return (
            <div className="cast-search-vis">
                <form onSubmit={this.searchAllocation}>
                    <label>Allocation ID: </label>
                    <input type="number" onChange={this.handleChange.bind(this, 'aid')}/> 
                    <input type="submit" value="Submit"/>
                </form>
            </div>
        )
    }



    componentDidMount(){
        this.props.renderComplete();
    }
}


export function CASTSearchVisWrapper(props) {
    return (
        <CASTSearchVisComponent
            fontSize={props.vis.params.fontSize}
            renderComplete={props.renderComplete}
            openLinksInNewTab={props.vis.params.openLinksInNewTab}
        /> 
    );
}
