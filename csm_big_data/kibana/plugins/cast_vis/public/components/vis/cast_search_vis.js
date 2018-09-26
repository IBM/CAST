import PropTypes from 'prop-types';
import React, { Component } from 'react';
import { uiModules } from 'ui/modules';
import { AllocationComponent } from './allocation_component';
import { JobComponent } from './job_component';
import { ResultsComponent } from './results_component';

import {
  EuiButton,
  EuiFlexGroup,
  EuiFlexItem,
  EuiFormRow,
  EuiKeyboardAccessible
} from '@elastic/eui';

export class CastSearchVis extends Component {
    constructor(props) {
        super(props);

        this.activeFilter={};
        this._resultComp={};

        this.resultsId  =  Date.now();


        this.handleSearch = this.handleSearch.bind(this);
        this.onFilterChange = this.onFilterChange.bind(this);
        this.handleSearchBarFilter = this.handleSearchBarFilter.bind(this);
        this.handleResultsRender = this.handleResultsRender.bind(this);
        this.handleSetTimeRange = this.handleSetTimeRange.bind(this);

        this.props.scope.API.queryFilter.on('update', this.onFilterChange);
    }

    destroy(){
        this.props.scope.API.queryFilter.off('update', this.onFilterChange);
    }

    onFilterChange(){

    }
    
    async handleSearch(query, search, callback) {
        const indexPattern = await this.props.scope.API.indexPatterns.get(search.indexPatternId);
        const searchSource = new this.props.scope.API.SearchSource()
                .inherits(false)
                .set('index', indexPattern)
                .set('version', true)
                .set('size', 100)
                .set('query', {
                    query,
                    language: 'lucene'
                });

        const resp = await searchSource.fetch().catch(e=>console.log(e));
        callback(resp);
    }

    handleSetTimeRange(startTime, endTime)
    {
        this.props.scope.API.timeFilter.time.mode = "absolute";
        this.props.scope.API.timeFilter.time.from = startTime ? startTime : "now";
        this.props.scope.API.timeFilter.time.to   = endTime ? endTime : "now";
    }
    

    handleSearchBarFilter(filter, searchId)
    {
        this.props.scope.API.queryFilter.removeFilter(this.filter);
        filter.label = "CAST Search Filter";
        filter.meta = {
            disabled: false,
            negate: false,
            alias : "CAST Search Filter"
        }

        this.filter = filter;
        this.props.scope.API.queryFilter.addFilters([filter]);
    }

    handleResultsRender(resultsMap )
    {
        if (resultsMap)
        {
            this._resultComp.updateResults(resultsMap, false);
        }
    }


    renderSearches() {
        // This function will rebuild the search components.
        return this.props.searches.map((search, index) => {
            let searchComponent =( <EuiFlexItem> </EuiFlexItem>);
            switch (search.type) {
                case 'allocation-id':
                    searchComponent = (
                        <AllocationComponent
                           search={search}
                           searchIndex={index}
                           onSearch={this.handleSearch}
                           handleFilter={this.handleSearchBarFilter}
                           renderResults={this.handleResultsRender}
                           handleTimeRange={this.handleSetTimeRange}
                        />
                    );
                    break;
                case 'job-id':
                    searchComponent = (
                        <JobComponent
                           search={search}
                           searchIndex={index}
                           onSearch={this.handleSearch}
                           handleFilter={this.handleSearchBarFilter}
                           renderResults={this.handleResultsRender}
                           handleTimeRange={this.handleSetTimeRange}
                        />
                    );
                    break;
                case 'custom':
                    break;
                default:
                    throw new Error(`Invalid search type ${search.type}`);
                }
            return (
                <EuiFlexItem
                  key={search.id}
                  style={{ minWidth:'250px'}}
                  >
                    {searchComponent}
                </EuiFlexItem>
                );

        });
    }
    
    render() {
        this._resultComp={};
        return (
            <div className="castSearchVis">
            <EuiFlexGroup wrap>
                {this.renderSearches()}

                <EuiFlexItem 
                    key={this.resultsId}
                    style={{ minWidth:'250px'}}>
                    <ResultsComponent
                        ref={(child) => { this._resultComp = child; }}
                    />
                </EuiFlexItem>
            </EuiFlexGroup>
            </div>
        );
    }
}

CastSearchVis.propTypes = {
    scope: PropTypes.object.isRequired,
    searches: PropTypes.array.isRequired,
}

export function CastSearchVisWrapper(props) {
    return <CastSearchVis/>
}
